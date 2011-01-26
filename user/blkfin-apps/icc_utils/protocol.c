/* Multicore communication on a BF561
 *
 * Copyright 2004-2009 Analog Devices Inc.
 * Licensed under the GPL-2 or later.
 */

#include <asm-generic/errno-base.h>
#include <linux/bitmap.h>
#include <icc.h>
#include "protocol.h"

struct coreb_icc_node coreb_info;
struct sm_message scratch_msg;
sm_uint16_t iccq_should_stop;

struct sm_task sm_task1;

struct sm_proto *sm_protos[SP_MAX];

void *get_free_buffer(sm_uint32_t size)
{
	return gen_pool_alloc(coreb_info.pool, size);
}

void free_buffer(sm_uint32_t addr, sm_uint32_t size)
{
	gen_pool_free(coreb_info.pool, addr, size);
}

int check_buffer_inpool(sm_uint32_t addr, sm_uint32_t size)
{
	return gen_pool_check(coreb_info.pool, addr, size);
}

int init_sm_session_table(void)
{
	coreb_info.icc_info.sessions_table = gen_pool_alloc(coreb_info.pool,
		sizeof(struct sm_session_table)); /* alloc session table*/
	if (!coreb_info.icc_info.sessions_table) {
		coreb_msg("@@@ alloc session table failed\n");
		return -ENOMEM;
	}
}

static int sm_message_enqueue(int dstcpu, int srccpu, struct sm_message_queue * msg)
{
	struct sm_message_queue *outqueue = &coreb_info.icc_info.icc_queue[dstcpu];
	sm_atomic_t sent = sm_atomic_read(&outqueue->sent);
	sm_atomic_t received = sm_atomic_read(&outqueue->received);
	if ((sent - received) >= (SM_MSGQ_LEN - 1)) {
		coreb_msg("over run\n");
		return -EAGAIN;
	}
	memcpy(&outqueue->messages[(sent%SM_MSGQ_LEN)], msg, sizeof(struct sm_message));
	sent++;
	sm_atomic_write(&outqueue->sent, sent);
	return 0;
}

static int sm_message_dequeue(int srccpu, struct sm_message_queue * msg)
{
	struct sm_message_queue *inqueue = &coreb_info.icc_info.icc_queue[srccpu];
	sm_atomic_t received = sm_atomic_read(&inqueue->received);
	received++;
	sm_atomic_write(&inqueue->received, received);
	return 0;
}

static struct sm_session_table* sm_get_session_table(void)
{
	struct sm_session_table *table = coreb_info.icc_info.sessions_table;
	table->refcnt++;
	return table;
}

static int sm_put_session_table(void)
{
	struct sm_session_table *table = coreb_info.icc_info.sessions_table;
	table->refcnt--;
	return 0;
}

static struct sm_session* sm_index_to_session(sm_uint32_t session_idx)
{
	struct sm_session *session;
	struct sm_session_table *table = coreb_info.icc_info.sessions_table;
	if (session_idx < 0 && session_idx >= MAX_SESSIONS)
		return NULL;
	session = &table->sessions[session_idx];
	return session;
}

static sm_uint32_t sm_session_to_index(struct sm_session *session)
{
	struct sm_session_table *table = coreb_info.icc_info.sessions_table;
	sm_uint32_t index;
	if ((session > &table->sessions[0])
		&& (session < &table->sessions[MAX_SESSIONS])) {
		return ((session - &table->sessions[0])/sizeof(struct sm_session));
	}
	return -EINVAL;
}

static int sm_send_message_internal(struct sm_message *msg, int dstcpu, int srccpu)
{
	int ret = 0;
	coreb_msg("%s() dst %d src %d\n", __func__, dstcpu, srccpu);
	ret = sm_message_enqueue(dstcpu, srccpu, msg);
	if (!ret)
		platform_send_ipi_cpu(dstcpu, IRQ_SUPPLE_0);
	return ret;
}

static sm_uint32_t sm_alloc_session(struct sm_session_table *table)
{
	sm_uint32_t index;
	index = find_next_zero_bit(table->bits, BITS_PER_LONG, 0);
	if (index >= BITS_PER_LONG)
		return -EAGAIN;
	bitmap_set(table->bits, index, 1);

	table->nfree--;
	return index;
}

static int sm_free_session(sm_uint32_t slot, struct sm_session_table *table)
{
	memset(&table->sessions[slot], 0, sizeof(struct sm_session));
	__clear_bit(slot, table->bits);
	table->nfree++;
	return 0;
}

static int
sm_find_session(sm_uint32_t local_ep, sm_uint32_t remote_ep,
			struct sm_session_table *table)
{
	sm_uint32_t index;
	struct sm_session *session;
//	coreb_msg("%s bits %08x\n", __func__, table->bits[0]);
	for_each_set_bit(index, table->bits, BITS_PER_LONG) {
		session = &table->sessions[index];
		coreb_msg("index %d ,local ep %d\n", index, session->local_ep);
		if(session->local_ep == local_ep) {
			if (remote_ep && session->remote_ep != remote_ep)
				return -EINVAL;
			goto found_slot;
		}
	}
	return -EINVAL;
found_slot:
	return index;
}

int sm_create_session(sm_uint32_t src_ep, sm_uint32_t type)
{
	coreb_msg("create ep \n");
	struct sm_session_table *table = sm_get_session_table();
	sm_uint32_t index = sm_alloc_session(table);
	if (index >=0 && index <32) {
		table->sessions[index].local_ep = src_ep;
		table->sessions[index].remote_ep = 0;
		table->sessions[index].pid = 0;
		table->sessions[index].flags = 0;
		table->sessions[index].type = type;
		table->sessions[index].proto_ops = sm_protos[type];
		INIT_LIST_HEAD(&table->sessions[index].bufs);
		INIT_LIST_HEAD(&table->sessions[index].messages);
		coreb_msg("create ep index %d srcep %d\n", index, src_ep);
		sm_put_session_table();
		return index;
	}
	sm_put_session_table();
	return -EAGAIN;
}

int sm_registe_session_handler(struct sm_session *session,
			void (*handle)(struct sm_message *msg))
{
	coreb_msg("%s\n", __func__);
	if (handle)
		session->handle= handle;
	return 0;
}

static int
sm_wait_for_connect_ack(struct sm_session *session)
{
	return 0;
}

int sm_destroy_session(sm_uint32_t src_ep)
{
	struct sm_session_table *table = sm_get_session_table();
	sm_uint32_t index = sm_find_session(src_ep, 0, table);
	int ret = 0;
	if (index < 0)
		ret = -EINVAL;

	sm_free_session(index, table);
	sm_put_session_table();
	return ret;
}

static int sm_connect_session(sm_uint32_t dst_ep, sm_uint32_t dst_cpu,
			sm_uint32_t src_ep)
{
	struct sm_session_table *table;
	struct sm_session *session;
	table = sm_get_session_table();
	sm_uint32_t index = sm_find_session(src_ep, 0, table);
	sm_put_session_table();
	session = sm_index_to_session(index);
	if (!session)
		return -EINVAL;

	sm_send_connect(session, dst_ep, dst_cpu);
	if (sm_wait_for_connect_ack(session))
		return -EAGAIN;
	table->sessions[index].remote_ep = dst_ep;
	table->sessions[index].flags = SM_CONNECT;
	sm_send_connect_done(session, dst_ep, dst_cpu);
	return 0;
}

static int sm_disconnect_session(sm_uint32_t dst_ep, sm_uint32_t src_ep)
{
	struct sm_session_table *table;
	table = sm_get_session_table();
	sm_uint32_t index = sm_find_session(src_ep, 0, table);
	sm_put_session_table();
	if (index >= MAX_SESSIONS)
		return -EINVAL;

	table->sessions[index].remote_ep = 0;
	table->sessions[index].flags = 0;
	return 0;
}

#define SM_MAX_TASKARGS 3
void sm_handle_control_message(sm_uint32_t cpu)
{

	struct sm_message_queue *inqueue = &coreb_info.icc_info.icc_queue[cpu];
	sm_atomic_t sent = sm_atomic_read(&inqueue->sent);
	sm_atomic_t received = sm_atomic_read(&inqueue->received);
	struct sm_message *msg;
	struct sm_task *task;
	char *task_argv[SM_MAX_TASKARGS];
	int i;
	msg = &inqueue->messages[(received % SM_MSGQ_LEN)];


	if ((SM_MSG_PROTOCOL(msg->type) == SP_CORE_CONTROL) || (SM_MSG_PROTOCOL(msg->type) == SP_TASK_MANAGER)) {

		coreb_msg("%s\n", __func__);
		switch (msg->type) {
		case SM_CORE_START:
			iccq_should_stop = 0;
			break;
		case SM_CORE_STOP:
			iccq_should_stop = 1;
			break;
		case SM_TASK_RUN:
			task = (struct sm_task*)msg->payload;
			memcpy(&sm_task1, msg->payload,
				sizeof(struct sm_task));
			/* task_ack; */
			for (i = 0; i < SM_MAX_TASKARGS; i++) {
				task_argv[i] = task->task_argv[i];
			}
			coreb_msg("task init %x\n", sm_task1.task_init);
			coreb_msg("task init %s\n", task_argv[0]);
			coreb_msg("task init %s\n", task_argv[1]);
			if (sm_task1.task_init)
				sm_task1.task_init(task->task_argc, task_argv);
			coreb_msg("finish %s\n", __func__);
			break;
		case SM_TASK_KILL:
			if (sm_task1.task_init)
				sm_task1.task_exit();
			break;
		}
		sm_message_dequeue(cpu, msg);
		return;
	}
}

int
sm_send_control_msg(struct sm_session *session, sm_uint32_t remote_ep,
			sm_uint32_t dst_cpu, sm_uint32_t payload,
			sm_uint32_t len, sm_uint32_t type)
{
	int ret;
	struct sm_message *m = &scratch_msg;
	memset(m, 0, sizeof(struct sm_message));

	m->type = type;
	m->src = blackfin_core_id();
	m->dst = dst_cpu;
	m->src_ep = session->local_ep;
	m->dst_ep = remote_ep;
	m->length = len;
	m->payload = payload;

	coreb_msg("ack dst %x, src %x\n", m->dst, m->src);

	ret = sm_send_message_internal(m, m->dst, m->src);
	if (ret)
		return -EAGAIN;
	return ret;

}

int
sm_send_packet_ack(struct sm_session *session, sm_uint32_t remote_ep,
		sm_uint32_t dst_cpu, sm_uint32_t payload, sm_uint32_t len)
{
	return sm_send_control_msg(session, remote_ep, dst_cpu, payload,
					len, SM_PACKET_CONSUMED);
}

int
sm_send_session_packet_ack(struct sm_session *session, sm_uint32_t remote_ep,
		sm_uint32_t dst_cpu, sm_uint32_t payload, sm_uint32_t len)
{
	return sm_send_control_msg(session, remote_ep, dst_cpu, payload,
					len, SM_SESSION_PACKET_COMSUMED);
}

int sm_send_connect(struct sm_session *session, sm_uint32_t remote_ep,
			sm_uint32_t dst_cpu)
{
	return sm_send_control_msg(session, remote_ep, dst_cpu, 0,
			0, SM_SESSION_PACKET_CONNECT);
}

int sm_send_connect_ack(struct sm_session *session, sm_uint32_t remote_ep,
			sm_uint32_t dst_cpu)
{
	return sm_send_control_msg(session, remote_ep, dst_cpu, 0,
			0, SM_SESSION_PACKET_CONNECT_ACK);
}

int sm_send_connect_done(struct sm_session *session, sm_uint32_t remote_ep,
			sm_uint32_t dst_cpu)
{
	return sm_send_control_msg(session, remote_ep, dst_cpu, 0,
			0, SM_SESSION_PACKET_CONNECT_DONE);
}

int sm_send_close(struct sm_session *session, sm_uint32_t remote_ep,
			sm_uint32_t dst_cpu)
{
	return sm_send_control_msg(session, remote_ep, dst_cpu, 0,
			0, SM_SESSION_PACKET_CLOSE);
}

int sm_send_close_ack(struct sm_session *session, sm_uint32_t remote_ep,
			sm_uint32_t dst_cpu)
{
	return sm_send_control_msg(session, remote_ep, dst_cpu, 0,
			0, SM_SESSION_PACKET_CLOSE_ACK);
}

int sm_send_error(struct sm_session *session, sm_uint32_t remote_ep,
			sm_uint32_t dst_cpu)
{
	return sm_send_control_msg(session, remote_ep, dst_cpu, 0,
			0, SM_PACKET_ERROR);
}

void *sm_request(sm_uint32_t size, struct sm_session *session)
{
	void *buf = get_free_buffer(size);
	return buf;
}

void sm_release(void *addr, sm_uint32_t size, struct sm_session *session)
{
	struct sm_message *msg = NULL;
	if (!list_empty(&session->messages)) {
		msg = list_first_entry(&session->messages,
					struct sm_message, next);
	}
	if (SM_MSG_PROTOCOL(msg->type) == SP_PACKET)
		sm_send_packet_ack(session, msg->src_ep, msg->src, msg->payload, msg->length);
	else
		sm_send_session_packet_ack(session, msg->src_ep, msg->src, msg->payload, msg->length);

	list_del(&msg->next);
}

int
sm_send_packet(sm_uint32_t session_idx, sm_uint32_t dst_ep,
		sm_uint32_t dst_cpu, void *buf, sm_uint32_t len)
{
	struct sm_session *session;
	struct sm_message *m;
	void *payload_buf = NULL;
	int ret = -EAGAIN;

	session = sm_index_to_session(session_idx);
	m = &scratch_msg;
	memset(m, 0, sizeof(struct sm_message));

	m->src_ep = session->local_ep;
	m->src = blackfin_core_id();
	m->dst_ep = dst_ep;
	m->dst = dst_cpu;
	m->length = len;
	m->type = SM_MSG_TYPE(session->type, 0);

	if (m->length) {
		if (!check_buffer_inpool(buf, len)) {
			payload_buf = get_free_buffer(m->length);
			if (!payload_buf) {
				ret = -ENOMEM;
				goto out;
			}
			m->payload = payload_buf;
			memcpy(m->payload, buf, m->length);
		} else {
			m->payload = buf;
			coreb_msg("%s() %s \n", __func__, m->payload);
		}
	} else {
		ret = -EINVAL;
		goto out;
	}
	ret = session->proto_ops->sendmsg(m, session);
	if (ret)
		goto fail;

	ret = sm_send_message_internal(m, m->dst, m->src);
	if (!ret)
		goto out;

fail:
	if (payload_buf)
		free_buffer(payload_buf, m->length);
out:
	return ret;
}

int sm_recv_packet(sm_uint32_t session_idx, void **buf,
				sm_uint32_t len)
{
	struct sm_message *msg;
	struct sm_session *session;
	int cpu = blackfin_core_id();
	int ret = 0;

	session = sm_index_to_session(session_idx);

	coreb_msg(" %s session type %x localep%d\n",__func__, session->type, session->local_ep);
	if (!list_empty(&session->messages)) {
		msg = list_first_entry(&session->messages,
					struct sm_message, next);
		coreb_msg("%s() %s\n", __func__, msg->payload);
		*buf = msg->payload;
		ret = msg->length;
		sm_message_dequeue(cpu, msg);
	} else {
		ret = -EINVAL;
	}
	coreb_msg(" %s msg\n",__func__);
	return ret;
}

int iccqueue_getpending(sm_uint32_t srccpu)
{
	struct sm_message_queue *inqueue = &coreb_info.icc_info.icc_queue[srccpu];
	sm_atomic_t sent = sm_atomic_read(&inqueue->sent);
	sm_atomic_t received = sm_atomic_read(&inqueue->received);
	sm_atomic_t pending;
//	coreb_msg("@@@sent=%d received=%d\n", sent, received);
	pending = sent - received;
	if(pending < 0)
		pending += USHRT_MAX;
	return (pending % SM_MSGQ_LEN);
}

static int msg_recv_internal(struct sm_message *msg, struct sm_session *session)
{
	struct sm_session_table *table = coreb_info.icc_info.sessions_table;
	sm_uint32_t ep_num = msg->dst_ep;
	int cpu = blackfin_core_id();
	int slot;
	struct sm_message *message;
	int ret = 0;

	coreb_msg("%s recv dstep %d,slot %d\n", __func__, ep_num, slot);
	sm_message_dequeue(cpu, msg);
	return ret;
}

static int sm_default_sendmsg(struct sm_message *msg, struct sm_session *session)
{
	coreb_msg("%s msg type %x\n", __func__, msg->type);
	switch (SM_MSG_PROTOCOL(msg->type)) {
	case SP_PACKET:
		break;
	case SP_SESSION_PACKET:
		break;
	case SM_PACKET_ERROR:
		coreb_msg("SM ERROR %08x\n", msg->payload);
		break;
	default:
		break;
	};
	return 0;
}

static int
sm_default_recvmsg(struct sm_message *msg, struct sm_session *session)
{
	sm_uint32_t ep_num = msg->dst_ep;
	int slot;
	int ret = 0;
	int cpu = blackfin_core_id();

	coreb_msg("type %x,dst %d, dstep %d, src %d, srcep %d \n", msg->type, msg->dst, msg->dst_ep, msg->src, msg->src_ep);
	switch (msg->type) {
	case SM_PACKET_CONSUMED:
	case SM_SESSION_PACKET_COMSUMED:
		coreb_msg("free buffer %x\n", msg->payload);
		free_buffer(msg->payload, 0x1000);
		break;
	case SM_SESSION_PACKET_CONNECT_ACK:
		session->remote_ep = msg->src_ep;
		session->flags = SM_CONNECT;
		break;
	case SM_SESSION_PACKET_CONNECT:
		session->remote_ep = msg->src_ep;
		session->flags = SM_CONNECTING;
		sm_send_connect_ack(session, msg->src_ep, msg->src);
		break;
	case SM_SESSION_PACKET_CONNECT_DONE:
		session->flags = SM_CONNECT;
		break;
	case SM_SESSION_PACKET_CLOSE:
		session->remote_ep = 0;
		session->flags = 0;
		sm_send_close_ack(session, msg->src_ep, msg->src);
	case SM_SESSION_PACKET_CLOSE_ACK:
		session->remote_ep = 0;
		session->flags = 0;
	case SM_PACKET_READY:
		coreb_msg("recved packet msg handle%x\n", (unsigned int)session->handle);
		if (session->handle)
			session->handle(msg, session);
		return ret;
	case SM_SESSION_PACKET_READY:
		if (session->handle)
			session->handle(msg, session);
		return ret;
	case SM_PACKET_ERROR:
		coreb_msg("SM ERROR %08x\n", msg->payload);
		break;
	default:
		ret = -EINVAL;
	};

	sm_message_dequeue(cpu, msg);
	return ret;
}

static int sm_default_shutdown(struct sm_session *session)
{
	return 0;
}

static int sm_default_error(struct sm_message *msg, struct sm_session *session)
{
	return 0;
}

struct sm_proto packet_proto = {
	.sendmsg = sm_default_sendmsg,
	.recvmsg = sm_default_recvmsg,
	.shutdown = sm_default_shutdown,
	.error = sm_default_error,
};

struct sm_proto session_packet_proto = {
	.sendmsg = sm_default_sendmsg,
	.recvmsg = sm_default_recvmsg,
	.shutdown = sm_default_shutdown,
	.error = sm_default_error,
};

void register_sm_proto(void)
{
	sm_protos[SP_PACKET] = &packet_proto;
	sm_protos[SP_SESSION_PACKET] = &session_packet_proto;
}

void msg_handle(void)
{
	int cpu = blackfin_core_id(); /* cpu_id(); */
	struct sm_message_queue *inqueue = &coreb_info.icc_info.icc_queue[cpu];
	sm_atomic_t sent = sm_atomic_read(&inqueue->sent);
	sm_atomic_t received = sm_atomic_read(&inqueue->received);
	struct sm_message *msg;
	struct sm_session *session;
	int pending;
	sm_uint32_t index;
	msg = &inqueue->messages[(received % SM_MSGQ_LEN)];

	index = sm_find_session(msg->dst_ep, 0, coreb_info.icc_info.sessions_table);

	session = sm_index_to_session(index);
	pending = iccqueue_getpending(cpu);
	if (!pending)
		return;
	if (session && (SM_MSG_PROTOCOL(msg->type) == session->type)) {
		list_add(&msg->next, &session->messages);
		if (session->proto_ops->recvmsg)
			session->proto_ops->recvmsg(msg, session);
		else
			coreb_msg("unsupported protocol\n");
	} else {
		sm_message_dequeue(cpu, msg);
		/* to do send error ack to cpu 0 */
	}
}

