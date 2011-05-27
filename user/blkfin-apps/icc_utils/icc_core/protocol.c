/* Multicore communication on a BF561
 *
 * Copyright 2004-2009 Analog Devices Inc.
 * Licensed under the GPL-2 or later.
 */

#include <asm-generic/errno-base.h>
#include <linux/bitmap.h>
#include <icc.h>
#include <protocol.h>
#include <blackfin.h>

static inline void coreb_idle(void)
{
	__asm__ __volatile__( \
			".align 8;" \
			"nop;"  \
			"nop;"  \
			"idle;" \
			: \
			:  \
			);
}

struct coreb_icc_node coreb_info;
struct sm_msg scratch_msg;
struct sm_message scratch_message;
sm_uint16_t iccq_should_stop;

#define SM_TASK_NONE 0
#define SM_TASK_INIT 1
#define SM_TASK_RUNNING 2
struct sm_task sm_task1;
int sm_task1_status = 0;
int sm_task1_control_ep = 0;
uint32_t sm_task1_msg_buffer = 0;

struct sm_proto *sm_protos[SP_MAX];

struct sm_message *get_message()
{
	return (struct sm_message *)gen_pool_alloc(coreb_info.msg_pool, 1 << 6);
}

void free_message(struct sm_message *message)
{
	gen_pool_free(coreb_info.msg_pool, (sm_uint32_t)message, 1 << 6);
}

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
	coreb_info.icc_info.sessions_table->nfree = MAX_ENDPOINTS; 
}

static int get_msg_src(struct sm_msg *msg)
{
	unsigned int n = 0;
	unsigned int offset;
	unsigned int align = 256;
	offset = (unsigned int)msg - MSGQ_START_ADDR;
	if (align < sizeof(struct sm_message_queue))
		align = (sizeof(struct sm_message_queue) + align - 1) / align;
	n = offset / align;
	if ((n % 2) == 0)
		return n + 1;
	else
		return 0;
}

static int sm_message_enqueue(int dstcpu, int srccpu, struct sm_msg *msg)
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

static int sm_message_dequeue(int srccpu, struct sm_msg *msg)
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

struct sm_session* sm_index_to_session(sm_uint32_t session_idx)
{
	struct sm_session *session;
	struct sm_session_table *table = coreb_info.icc_info.sessions_table;
	if (session_idx < 0 && session_idx >= MAX_SESSIONS)
		return NULL;
	if (!test_bit(session_idx, table->bits))
		return NULL;
	session = &table->sessions[session_idx];
	return session;
}

sm_uint32_t sm_session_to_index(struct sm_session *session)
{
	struct sm_session_table *table = coreb_info.icc_info.sessions_table;
	sm_uint32_t index;
	if ((session >= &table->sessions[0])
		&& (session < &table->sessions[MAX_SESSIONS])) {
		return ((session - &table->sessions[0])/sizeof(struct sm_session));
	}
	return -EINVAL;
}

static int sm_send_message_internal(struct sm_msg *msg, int dstcpu, int srccpu)
{
	int ret = 0;
	coreb_msg("%s() dst %d src %d %x\n", __func__, dstcpu, srccpu, msg->type);
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

int sm_find_session(sm_uint32_t local_ep, sm_uint32_t remote_ep,
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
	sm_uint32_t index = sm_find_session(src_ep, 0, table);
	if (index >= 0 && index < 32) {
		coreb_msg("already bound index %d srcep %d\n", index, src_ep);
		return -EEXIST;
	}
	if (type >= SP_MAX) {
		coreb_msg("bad type %x\n", type);
		return -EINVAL;
	}
	index = sm_alloc_session(table);
	if (index >=0 && index <32) {
		table->sessions[index].local_ep = src_ep;
		table->sessions[index].remote_ep = 0;
		table->sessions[index].pid = 0;
		table->sessions[index].flags = 0;
		table->sessions[index].n_uncompleted = 0;
		table->sessions[index].n_avail = 0;
		table->sessions[index].type = type;
		table->sessions[index].proto_ops = sm_protos[type];
		INIT_LIST_HEAD(&table->sessions[index].tx_messages);
		INIT_LIST_HEAD(&table->sessions[index].rx_messages);
		coreb_msg("create ep index %d srcep %d\n", index, src_ep);
		sm_put_session_table();
		return index;
	}
	sm_put_session_table();
	return -EAGAIN;
}

int sm_register_session_handler(sm_uint32_t session_idx,
			void (*handle)(struct sm_message *message, struct sm_session *session))
{
	struct sm_session *session = sm_index_to_session(session_idx);
	if (!session)
		return -EINVAL;

	if (handle)
		session->handle = handle;

	coreb_msg("%s handle %x\n", __func__, session->handle);
	return 0;
}

static int
sm_wait_for_connect_ack(struct sm_session *session)
{
	return 0;
}

int sm_destroy_session(sm_uint32_t session_idx)
{
	struct sm_message *message;
	struct sm_msg *msg;
	struct sm_session *session;
	struct sm_session_table *table;
	session = sm_index_to_session(session_idx);
	if (!session)
		return -EINVAL;
	while (!list_empty(&session->rx_messages)) {
		message = list_first_entry(&session->rx_messages,
				struct sm_message, next);
		msg = &message->msg;

		if (session->flags == SM_CONNECT)
			sm_send_session_packet_ack(session, msg->src_ep,
					message->src, msg->payload, msg->length);
		else
			sm_send_packet_ack(session, msg->src_ep,
					message->src, msg->payload, msg->length);
		list_del(&message->next);
		free_message(message);
	}

	if (session->flags == SM_CONNECT)
		sm_send_close(session, msg->src_ep, message->src);

	table = sm_get_session_table();
	sm_free_session(session_idx, table);
	sm_put_session_table();
	return 0;
}

int sm_connect_session(sm_uint32_t dst_ep, sm_uint32_t dst_cpu,
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

int sm_disconnect_session(sm_uint32_t dst_ep, sm_uint32_t src_ep)
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

int sm_open_session(sm_uint32_t index)
{
	struct sm_session *session;
	session = sm_index_to_session(index);
	if (!session)
		return -EINVAL;
	if (session->flags == SM_CONNECT) {
		session->flags |= SM_OPEN;
		return 0;
	}
	return -EINVAL;
}

int sm_close_session(sm_uint32_t index)
{
	struct sm_session *session;
	session = sm_index_to_session(index);
	if (!session)
		return -EINVAL;
	if (session->flags & SM_OPEN) {
		session->flags &= ~SM_OPEN;
		return 0;
	}
	return -EINVAL;
}

#define SM_MAX_TASKARGS 3
void sm_handle_control_message(sm_uint32_t cpu)
{

	struct sm_message_queue *inqueue = &coreb_info.icc_info.icc_queue[cpu];
	sm_atomic_t sent = sm_atomic_read(&inqueue->sent);
	sm_atomic_t received = sm_atomic_read(&inqueue->received);
	struct sm_msg *msg;
	msg = &inqueue->messages[(received % SM_MSGQ_LEN)];

	coreb_msg("%s type %x\n", __func__, msg->type);

	if ((SM_MSG_PROTOCOL(msg->type) == SP_CORE_CONTROL) || (SM_MSG_PROTOCOL(msg->type) == SP_TASK_MANAGER)) {

		coreb_msg("%s %x %x\n", __func__, msg->type, SM_TASK_RUN);
		switch (msg->type) {
		case SM_CORE_START:
			iccq_should_stop = 0;
			break;
		case SM_CORE_STOP:
			iccq_should_stop = 1;
			break;
		case SM_TASK_RUN:
			if (sm_task1.task_init)
				sm_task1.task_exit();
			memset(&sm_task1, 0, sizeof(struct sm_task));

			memcpy(&sm_task1, msg->payload,
				sizeof(struct sm_task));
			coreb_msg("task init %x\n", sm_task1.task_init);
			sm_task1_control_ep = msg->src_ep;
			sm_task1_msg_buffer = msg->payload;
			if (sm_task1.task_init)
				sm_task1_status = SM_TASK_INIT;

			sm_send_task_run_ack(sm_task1_control_ep, cpu ^ 1);

			delay(1);

			coreb_msg("finish %s\n", __func__);
			break;
		case SM_TASK_KILL:
			if (sm_task1.task_init)
				sm_task1.task_exit();
			memset(&sm_task1, 0, sizeof(struct sm_task));
			sm_task1_status = SM_TASK_NONE;
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
	struct sm_msg *m = &scratch_msg;
	memset(m, 0, sizeof(struct sm_msg));

	m->type = type;
	if (session)
		m->src_ep = session->local_ep;
	m->dst_ep = remote_ep;
	m->length = len;
	m->payload = payload;

	ret = sm_send_message_internal(m, dst_cpu, blackfin_core_id());
	if (ret)
		return -EAGAIN;
	return ret;
}

int sm_send_task_run_ack(sm_uint32_t remote_ep,
		sm_uint32_t dst_cpu)
{
	int ret;
	struct sm_msg *m = &scratch_msg;
	memset(m, 0, sizeof(struct sm_msg));

	m->type = SM_TASK_RUN_ACK;
	m->src_ep = 0;
	m->dst_ep = remote_ep;
	m->length = 0;
	if (sm_task1_msg_buffer)
		m->payload = sm_task1_msg_buffer;

	ret = sm_send_message_internal(m, dst_cpu, blackfin_core_id());
	if (ret)
		return -EAGAIN;
	return ret;
}

int sm_send_task_kill_ack(struct sm_session *session, sm_uint32_t remote_ep,
		sm_uint32_t dst_cpu)
{
	int ret;
	struct sm_msg *m = &scratch_msg;
	memset(m, 0, sizeof(struct sm_msg));

	m->type = SM_TASK_KILL_ACK;
	m->src_ep = 0;
	m->dst_ep = remote_ep;
	m->length = 0;
	m->payload = 0;

	ret = sm_send_message_internal(m, dst_cpu, blackfin_core_id());
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
					len, SM_SESSION_PACKET_CONSUMED);
}

int sm_send_scalar_cmd(struct sm_session *session, sm_uint32_t remote_ep,
		sm_uint32_t dst_cpu, sm_uint32_t payload, sm_uint32_t len)
{
	return sm_send_control_msg(session, remote_ep, dst_cpu, payload,
			len, SM_SCALAR_READY_64);
}

int sm_send_scalar_ack(struct sm_session *session, sm_uint32_t remote_ep,
		sm_uint32_t dst_cpu, sm_uint32_t payload, sm_uint32_t len)
{
	return sm_send_control_msg(session, remote_ep, dst_cpu, payload,
			len, SM_SCALAR_CONSUMED);
}

	int
sm_send_session_scalar_ack(struct sm_session *session, sm_uint32_t remote_ep,
		sm_uint32_t dst_cpu, sm_uint32_t payload, sm_uint32_t len)
{
	return sm_send_control_msg(session, remote_ep, dst_cpu, payload,
			len, SM_SESSION_SCALAR_CONSUMED);
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

int sm_send_session_active(struct sm_session *session, sm_uint32_t remote_ep,
		sm_uint32_t dst_cpu)
{
	return sm_send_control_msg(session, remote_ep, dst_cpu, 0,
			0, SM_SESSION_PACKET_ACTIVE);
}

int sm_send_session_active_ack(struct sm_session *session, sm_uint32_t remote_ep,
		sm_uint32_t dst_cpu)
{
	return sm_send_control_msg(session, remote_ep, dst_cpu, SM_OPEN,
			0, SM_SESSION_PACKET_ACTIVE_ACK);
}

int sm_send_session_active_noack(struct sm_session *session, sm_uint32_t remote_ep,
		sm_uint32_t dst_cpu)
{
	return sm_send_control_msg(session, remote_ep, dst_cpu, 0,
			0, SM_SESSION_PACKET_ACTIVE_ACK);
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

void *sm_send_request(sm_uint32_t size, sm_uint32_t session_index)
{
	void *buf = get_free_buffer(size);
	return buf;
}

int sm_recv_release(void *addr, sm_uint32_t size, sm_uint32_t session_idx)
{
	struct sm_message *message = NULL;
	struct sm_msg *msg = NULL;
	struct sm_session *session = sm_index_to_session(session_idx);
	if (!list_empty(&session->rx_messages)) {
		message = list_first_entry(&session->rx_messages,
					struct sm_message, next);
		msg = &message->msg;

	}

	if (msg && msg->payload != addr)
		return -EINVAL;

	if (SM_MSG_PROTOCOL(msg->type) == SP_PACKET)
		sm_send_packet_ack(session, msg->src_ep, message->src, msg->payload, msg->length);
	else
		sm_send_session_packet_ack(session, msg->src_ep, message->src, msg->payload, msg->length);

	list_del(&message->next);
	session->n_avail--;
	coreb_msg("free message %x\n", (unsigned long)message);
	free_message(message);
	return 0;
}

int
sm_send_scalar(sm_uint32_t session_idx, sm_uint16_t dst_ep,
		sm_uint16_t dst_cpu, sm_uint32_t scalar0, sm_uint32_t scalar1, sm_uint32_t size)
{
	struct sm_session *session;
	int ret = -EAGAIN;
	struct sm_message *message = get_message();

	session = sm_index_to_session(session_idx);

	message->msg.src_ep = session->local_ep;
	message->msg.dst_ep = dst_ep;
	message->msg.payload = scalar0;
	message->msg.length = scalar1;

	switch (size) {
	case 1:
		message->msg.type = SM_SCALAR_READY_8;
		break;
	case 2:
		message->msg.type = SM_SCALAR_READY_16;
		break;
	case 4:
		message->msg.type = SM_SCALAR_READY_32;
		break;
	case 8:
		message->msg.type = SM_SCALAR_READY_64;
		break;
	}

	ret = session->proto_ops->sendmsg(message, session);
	if (ret)
		goto fail;

	ret = sm_send_message_internal(&message->msg, dst_cpu, blackfin_core_id());
	if (!ret)
		goto out;

fail:
	free_message(message);
out:
	return ret;
}

int
sm_send_packet(sm_uint32_t session_idx, sm_uint16_t dst_ep,
		sm_uint16_t dst_cpu, void *buf, sm_uint32_t len)
{
	struct sm_session *session;
	void *payload_buf = NULL;
	int ret = -EAGAIN;
	struct sm_message *message = get_message();

	session = sm_index_to_session(session_idx);

	message->msg.src_ep = session->local_ep;
	message->msg.dst_ep = dst_ep;
	message->msg.length = len;
	message->msg.type = SM_MSG_TYPE(session->type, 0);

	if (message->msg.length) {
		if (!check_buffer_inpool(buf, len)) {
			payload_buf = get_free_buffer(message->msg.length);
			if (!payload_buf) {
				ret = -ENOMEM;
				goto out;
			}
			message->msg.payload = payload_buf;
			memcpy(message->msg.payload, buf, message->msg.length);
		} else {
			message->msg.payload = buf;
			coreb_msg("%s() %s \n", __func__, message->msg.payload);
		}
	} else {
		ret = -EINVAL;
		goto out;
	}
	ret = session->proto_ops->sendmsg(message, session);
	if (ret)
		goto fail;

	ret = sm_send_message_internal(&message->msg, dst_cpu, blackfin_core_id());
	if (!ret)
		goto out;

fail:
	free_message(message);
	if (payload_buf)
		free_buffer(payload_buf, message->msg.length);
out:
	return ret;
}

int sm_recv_scalar(sm_uint32_t session_idx, sm_uint16_t *src_ep, sm_uint16_t *src_cpu, sm_uint32_t *scalar0,
				sm_uint32_t *scalar1, sm_uint32_t *size)
{
	struct sm_message *message;
	struct sm_msg *msg;
	struct sm_session *session;
	int cpu = blackfin_core_id();
	int ret = 0;
	uint32_t len = 0;

	session = sm_index_to_session(session_idx);

	coreb_msg(" %s session type %x localep%d\n",__func__, session->type, session->local_ep);
	if (!list_empty(&session->rx_messages)) {
		message = list_first_entry(&session->rx_messages,
					struct sm_message, next);
		msg = &message->msg;

		coreb_msg(" msg type %x\n", msg->type);
		if (src_ep)
			*src_ep = message->msg.src_ep;
		if (src_cpu)
			*src_cpu = message->src;
		if (scalar0)
			*scalar0 = msg->payload;
		if (scalar1)
			*scalar1 = msg->length;

		switch (msg->type) {
		case SM_SCALAR_READY_8:
			len = 1;
			break;
		case SM_SCALAR_READY_16:
			len = 2;
			break;
		case SM_SCALAR_READY_32:
			len = 4;
			break;
		case SM_SCALAR_READY_64:
			len = 8;
			break;
		}

		if (size)
			*size = len;

		if (SM_MSG_PROTOCOL(msg->type) == SP_SCALAR)
			sm_send_scalar_ack(session, msg->src_ep, message->src,
					msg->payload, msg->length);
		else if (SM_MSG_PROTOCOL(msg->type) == SP_SESSION_SCALAR)
			sm_send_session_scalar_ack(session, msg->src_ep, message->src,
					msg->payload, msg->length);

		list_del(&message->next);
		session->n_avail--;
		coreb_msg("%s() s0%x s1%x avail %d\n", __func__, *scalar0, *scalar1, session->n_avail);
		ret = 1;


	} else {
		ret = -EAGAIN;
	}
	coreb_msg(" %s msg\n",__func__);
	return ret;
}

int sm_recv_packet(sm_uint32_t session_idx, sm_uint16_t *src_ep, sm_uint16_t *src_cpu, void **buf,
				sm_uint32_t *len)
{
	struct sm_message *message;
	struct sm_msg *msg;
	struct sm_session *session;
	int cpu = blackfin_core_id();
	int ret = 0;

	session = sm_index_to_session(session_idx);

	coreb_msg(" %s session type %x localep%d\n",__func__, session->type, session->local_ep);
	if (!list_empty(&session->rx_messages)) {
		message = list_first_entry(&session->rx_messages,
					struct sm_message, next);
		msg = &message->msg;
		if (src_ep)
			*src_ep = message->msg.src_ep;
		if (src_cpu)
			*src_cpu = message->src;
		if (len)
			*len = message->msg.length;
		coreb_msg("%s() %s\n", __func__, msg->payload);
		*buf = msg->payload;
		ret = msg->length;
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
	pending = sent - received;
	if(pending < 0)
		pending += USHRT_MAX;
	return (pending % SM_MSGQ_LEN);
}

static int msg_recv_internal(struct sm_msg *msg, struct sm_session *session)
{
	int ret = 0;
	int cpu = blackfin_core_id();
	struct sm_message *message = get_message();
	coreb_msg("%s msg type %x alloc %x\n", __func__, msg->type, (unsigned long)message);
	memcpy(&message->msg, msg, sizeof(struct sm_msg));
	message->dst = cpu;
	message->src = cpu ^ 1;
	if (session->handle) {
		coreb_msg("default handler\n");
		session->handle(message, session);
		free_message(message);
	} else {
		list_add(&message->next, &session->rx_messages);
		session->n_avail++;
		coreb_msg("avail %d \n", session->n_avail);
	}
	return ret;
}

static int sm_default_sendmsg(struct sm_message *message, struct sm_session *session)
{
	struct sm_msg *msg = &message->msg;
	coreb_msg("%s msg type %x\n", __func__, msg->type);
	switch (SM_MSG_PROTOCOL(msg->type)) {
	case SP_PACKET:
	case SP_SESSION_PACKET:
	case SP_SCALAR:
	case SP_SESSION_SCALAR:
		list_add(&message->next, &session->tx_messages);
		session->n_uncompleted++;
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
sm_default_recvmsg(struct sm_msg *msg, struct sm_session *session)
{
	int ret = 0;
	int cpu = blackfin_core_id();
	struct sm_message *uncompleted;

	coreb_msg("type %x, dstep %d, srcep %d \n", msg->type, msg->dst_ep, msg->src_ep);
	switch (msg->type) {
	case SM_PACKET_CONSUMED:
	case SM_SESSION_PACKET_CONSUMED:
		list_for_each_entry(uncompleted, &session->tx_messages, next) {
			if (uncompleted->msg.payload == msg->payload) {
				coreb_msg("ack matched free buf %x\n", msg->payload);
				goto matched;
			}
			coreb_msg("unmatched ack %08x %x uncomplete tx %08x\n", msg->payload, msg->length, uncompleted->msg.payload);
		}
		coreb_msg("unmatched ack\n");
		break;
matched:
		list_del(&uncompleted->next);
		session->n_uncompleted--;
		coreb_msg("free buffer %x\n", msg->payload);
		free_buffer(msg->payload, uncompleted->msg.length);
		free_message(uncompleted);
		break;
	case SM_SCALAR_CONSUMED:
	case SM_SESSION_SCALAR_CONSUMED:
		list_for_each_entry(uncompleted, &session->tx_messages, next) {
			if (uncompleted->msg.payload == msg->payload) {
				coreb_msg("ack matched free buf %x\n", msg->payload);
				goto matched1;
			}
			coreb_msg("unmatched ack %08x %x uncomplete tx %08x\n", msg->payload, msg->length, uncompleted->msg.payload);
		}
		coreb_msg("unmatched ack\n");
		break;
matched1:
		list_del(&uncompleted->next);
		session->n_uncompleted--;
		coreb_msg("free message %x\n", uncompleted);
		free_message(uncompleted);
		break;
	case SM_SESSION_PACKET_CONNECT_ACK:
		session->remote_ep = msg->src_ep;
		session->flags = SM_CONNECT;
		break;
	case SM_SESSION_PACKET_CONNECT:
		session->remote_ep = msg->src_ep;
		session->flags = SM_CONNECTING;
		session->type = SP_SESSION_PACKET;
		sm_send_connect_ack(session, msg->src_ep, cpu ^ 1);
		break;
	case SM_SESSION_PACKET_CONNECT_DONE:
		session->flags = SM_CONNECT;
		coreb_msg("connected %x %d\n", session->flags, session->remote_ep);
		break;
	case SM_SESSION_PACKET_ACTIVE:
		if (session->flags & SM_OPEN)
			sm_send_session_active_ack(session, msg->src_ep, cpu ^ 1);
		else
			sm_send_session_active_noack(session, msg->src_ep, cpu ^ 1);
		break;
	case SM_SESSION_PACKET_ACTIVE_ACK:
		if (session->flags & SM_OPEN) {
			if (msg->payload == SM_OPEN) {
				session->flags |= SM_ACTIVE;
			}
		}
		break;
	case SM_SESSION_PACKET_CLOSE:
		session->remote_ep = 0;
		session->flags = 0;
		sm_send_close_ack(session, msg->src_ep, cpu ^ 1);
	case SM_SESSION_PACKET_CLOSE_ACK:
		session->remote_ep = 0;
		session->flags = 0;
	case SM_PACKET_READY:
		coreb_msg("recved packet msg handle%x\n", (unsigned int)session->handle);
		msg_recv_internal(msg, session);
		break;
	case SM_SESSION_PACKET_READY:
		msg_recv_internal(msg, session);
		break;
	case SM_SCALAR_READY_8:
	case SM_SCALAR_READY_16:
	case SM_SCALAR_READY_32:
	case SM_SCALAR_READY_64:
	case SM_SESSION_SCALAR_READY_8:
	case SM_SESSION_SCALAR_READY_16:
	case SM_SESSION_SCALAR_READY_32:
	case SM_SESSION_SCALAR_READY_64:
		msg_recv_internal(msg, session);
		break;
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

static int sm_default_error(struct sm_message *message, struct sm_session *session)
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

struct sm_proto scalar_proto = {
	.sendmsg = sm_default_sendmsg,
	.recvmsg = sm_default_recvmsg,
	.shutdown = sm_default_shutdown,
	.error = sm_default_error,
};

struct sm_proto session_scalar_proto = {
	.sendmsg = sm_default_sendmsg,
	.recvmsg = sm_default_recvmsg,
	.shutdown = sm_default_shutdown,
	.error = sm_default_error,
};

void register_sm_proto(void)
{
	sm_protos[SP_PACKET] = &packet_proto;
	sm_protos[SP_SESSION_PACKET] = &session_packet_proto;
	sm_protos[SP_SCALAR] = &scalar_proto;
	sm_protos[SP_SESSION_SCALAR] = &session_scalar_proto;
}

void icc_run_task(void)
{
	char *task_argv[SM_MAX_TASKARGS];
	struct sm_task *task;
	int i;
	int cpu = blackfin_core_id();
	task = &sm_task1;
	for (i = 0; i < SM_MAX_TASKARGS; i++) {
		task_argv[i] = task->task_argv[i];
	}

	coreb_msg("before run task\n");
	if (sm_task1_status == SM_TASK_INIT) {
		sm_task1_status = SM_TASK_RUNNING;
		coreb_msg("before run task1\n");
		sm_task1.task_init(sm_task1.task_argc, task_argv);
	}
	coreb_idle();
}

int sm_get_session_status(void *user_param, uint32_t session_idx)
{
	int ret = 0;
	struct sm_session_status *param = (struct sm_session_status *)user_param;
	struct sm_session *session = sm_index_to_session(session_idx);
	if (!session)
		return -EINVAL;

	param->avail = session->n_avail;
	param->uncomplete = session->n_uncompleted;
	param->status = session->flags;

	return ret;
}

int icc_handle_scalar_cmd(struct sm_msg *msg)
{
	int ret;
	uint32_t scalar0, scalar1;
	uint16_t src_cpu;
	struct sm_session *session;
	int index;

	if (msg->type != SM_SCALAR_READY_64)
		return 0;

	scalar0 = msg->payload;
	scalar1 = msg->length;

	src_cpu = get_msg_src(msg);

	coreb_msg("scalar cmd %x %d\n", scalar0, src_cpu);

	if (SM_SCALAR_CMD(scalar0) != SM_SCALAR_CMD_HEAD)
		return 0;

	sm_send_scalar_ack(NULL, msg->src_ep, src_cpu,
			msg->payload, msg->length);

	coreb_msg("scalar cmd %x %x\n", SM_SCALAR_CMD(scalar0), SM_SCALAR_CMD_HEAD);
	switch (SM_SCALAR_CMDARG(scalar0)) {
	case SM_SCALAR_CMD_GET_SESSION_ID:
		index = sm_find_session(scalar1, 0, coreb_info.icc_info.sessions_table);
		session = sm_index_to_session(index);
		if (session) {
			scalar0 = MK_SM_SCALAR_CMD_ACK(SM_SCALAR_CMD_GET_SESSION_ID);
			scalar1 = index;
			sm_send_scalar_cmd(NULL, msg->src_ep, src_cpu, scalar0,
					scalar1);
		}
		break;
	case SM_SCALAR_CMD_GET_SESSION_TYPE:
		break;
	default:
		return 0;
	}

	return 1;
}

uint32_t msg_handle(void);
int icc_wait(int session_mask)
{
	int cpu = blackfin_core_id();
	int pending = iccqueue_getpending(cpu);
	uint32_t avail = 0;
	if (!pending) {
		coreb_idle();
		coreb_msg("@@@ wake up\n");
		return 0;
	}
	avail = msg_handle();
	return avail;
}

uint32_t msg_handle(void)
{
	int cpu = blackfin_core_id(); /* cpu_id(); */
	struct sm_message_queue *inqueue = &coreb_info.icc_info.icc_queue[cpu];
	sm_atomic_t sent = sm_atomic_read(&inqueue->sent);
	sm_atomic_t received = sm_atomic_read(&inqueue->received);
	struct sm_msg *msg;
	struct sm_session *session;
	int pending;
	struct sm_session_status sstatus;
	sm_uint32_t index;
	uint32_t avail = 0;
	msg = &inqueue->messages[(received % SM_MSGQ_LEN)];

	if (icc_handle_scalar_cmd(msg)) {
		sm_message_dequeue(cpu, msg);
		return;
	}

	index = sm_find_session(msg->dst_ep, 0, coreb_info.icc_info.sessions_table);

	session = sm_index_to_session(index);
	if (!session) {
		sm_message_dequeue(cpu, msg);
		return;
	}

	pending = iccqueue_getpending(cpu);
	if (!pending) {
		coreb_msg("BUG\n");
		return 0;
	}

	coreb_msg("msg type %x index %d session type %x\n", msg->type, index, session->type);
	if (session && (SM_MSG_PROTOCOL(msg->type) == session->type)) {
		if (session->proto_ops->recvmsg) {
			session->proto_ops->recvmsg(msg, session);
			sm_get_session_status(&sstatus, index);
			avail = sstatus.avail;
		} else
			coreb_msg("unsupported protocol\n");

	} else {
		coreb_msg("discard msg type\n", msg->type);
		sm_message_dequeue(cpu, msg);
	}
	return avail;
}

