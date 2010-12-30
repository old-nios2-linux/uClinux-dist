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

struct sm_proto *sm_protos[SP_MAX];

void *get_free_buffer(sm_uint32_t size)
{
	return gen_pool_alloc(coreb_info.pool, size);
}

void free_buffer(sm_uint32_t addr, sm_uint32_t size)
{
	gen_pool_free(coreb_info.pool, addr, size);
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

static int sm_send_message_internal(struct sm_message *msg, int dstcpu, int srccpu)
{
	int ret = 0;
	ret = sm_message_enqueue(dstcpu, srccpu, msg);
	if (!ret)
		platform_send_ipi_cpu(dstcpu, IRQ_SUPPLE_0);
	return ret;
}

static int
sm_send_message(sm_uint32_t session_idx, sm_uint32_t dst_ep, sm_uint32_t dst_cpu,
		void *buf, sm_uint32_t len, struct sm_session_table *table)
{
	struct sm_session *session;
	struct sm_message *m;
	void *payload_buf = NULL;
	int ret = -EAGAIN;
	if (session_idx < 0)
		return -EINVAL;
	session = &table->sessions[session_idx];
	m = &scratch_msg;
	memset(m, 0, sizeof(struct sm_message));

	m->src_ep = session->local_ep;
	m->src = blackfin_core_id();
	m->dst_ep = dst_ep;
	m->dst = dst_cpu;
	m->length = len;
	m->type = SM_MSG_TYPE(session->type, 0);

	if (m->length) {
		payload_buf = get_free_buffer(m->length);
		if (!payload_buf) {
			ret = -ENOMEM;
			goto out;
		}

		m->payload = payload_buf;

		memcpy(m->payload, buf, m->length);

		ret = session->proto_ops->sendmsg(m, session);
		if (ret)
			goto fail;

		ret = sm_send_message_internal(m, m->dst, m->src);
		if (ret)
			goto fail;

	} else {
		ret = -EINVAL;
		goto out;
	}

fail:
	free_buffer(payload_buf, m->length);
out:
	return ret;
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

int sm_send_close(struct sm_session *session, sm_uint32_t remote_ep,
			sm_uint32_t dst_cpu)
{
	return sm_send_control_msg(session, remote_ep, dst_cpu, 0,
			0, SM_SESSION_PACKET_CLOSE);
}

int sm_send_connect_ack(struct sm_session *session, sm_uint32_t remote_ep,
			sm_uint32_t dst_cpu)
{
	return sm_send_control_msg(session, remote_ep, dst_cpu, 0,
			0, SM_SESSION_PACKET_CONNECT_ACK);
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

static sm_uint32_t sm_get_session(struct sm_session_table *table)
{
	sm_uint32_t index;
	index = find_next_zero_bit(table->bits, BITS_PER_LONG, 0);
	if (index >= BITS_PER_LONG)
		return -EAGAIN;
	bitmap_set(table->bits, index, 1);

	table->nfree--;
	return index;
}

static int sm_put_session(sm_uint32_t slot, struct sm_session_table *table)
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
	coreb_msg("%s bits %08x\n", __func__, table->bits[0]);
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

static int sm_create_session(sm_uint32_t src_ep, sm_uint32_t type,
			struct sm_session_table *table)
{
	sm_uint32_t slot = sm_get_session(table);
	if (slot >=0 && slot <32) {
		table->sessions[slot].local_ep = src_ep;
		table->sessions[slot].remote_ep = 0;
		table->sessions[slot].pid = 0;
		table->sessions[slot].flags = 0;
		table->sessions[slot].type = type;
		table->sessions[slot].proto_ops = sm_protos[type];
		INIT_LIST_HEAD(&table->sessions[slot].messages);
		coreb_msg("create ep slot %d srcep %d\n", slot, src_ep);
		return slot;
	}
	return -EAGAIN;
}

static int sm_registe_session_handler(struct sm_session *session,
			void (*handle)(struct sm_message *msg))
{
	if (handle)
		session->handle= handle;
	return 0;
}

static int
sm_wait_for_connect_ack(struct sm_session *session)
{
	return 0;
}

static int sm_destroy_session(sm_uint32_t src_ep, struct sm_session_table *table)
{
	sm_uint32_t slot = sm_find_session(src_ep, 0, table);
	if (slot < 0)
		return -EINVAL;

	sm_put_session(slot, table);
	return 0;
}

static int sm_connect_session(sm_uint32_t dst_ep, sm_uint32_t dst_cpu,
			sm_uint32_t src_ep, struct sm_session_table *table)
{
	struct sm_session *session;
	sm_uint32_t slot = sm_find_session(src_ep, 0, table);
	if (slot >= 32)
		return -EINVAL;
	session = &table->sessions[slot];
	sm_send_connect(src_ep, dst_ep, dst_cpu);
	if (sm_wait_for_connect_ack(session))
		return -EAGAIN;
	table->sessions[slot].remote_ep = dst_ep;
	table->sessions[slot].flags = SM_CONNECT;
	return 0;
}

static int sm_disconnect_session(sm_uint32_t dst_ep, sm_uint32_t src_ep,
					struct sm_session_table *table)
{
	sm_uint32_t slot = sm_find_session(src_ep, 0, table);
	if (slot < 0)
		return -EINVAL;

	table->sessions[slot].remote_ep = 0;
	table->sessions[slot].flags = 0;
}

static int sm_recv_message(struct sm_message *msg, struct sm_message *tmp,
				struct sm_session *session)
{
	int cpu = blackfin_core_id();
	memcpy(tmp, msg, sizeof(struct sm_message));
	sm_message_dequeue(cpu, msg);
	return 0;
}

int iccqueue_getpending(sm_uint32_t srccpu)
{
	struct sm_message_queue *inqueue = &coreb_info.icc_info.icc_queue[srccpu];
	sm_atomic_t sent = sm_atomic_read(&inqueue->sent);
	sm_atomic_t received = sm_atomic_read(&inqueue->received);
	sm_atomic_t pending;
	coreb_msg("@@@sm msgq sent=%d received=%d\n", sent, received);
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
		free_buffer(msg->payload, 0x1000);
		break;
	case SM_SESSION_PACKET_CONNECT_ACK:
		session->remote_ep = msg->src_ep;
		session->flags = SM_CONNECT;
		break;
	case SM_SESSION_PACKET_CONNECT:
		session->remote_ep = msg->src_ep;
		session->flags = SM_CONNECT;
		sm_send_connect_ack(session, msg->src_ep, msg->src);
		break;
	case SM_SESSION_PACKET_CLOSE:
		session->remote_ep = 0;
		session->flags = 0;
		sm_send_close_ack(session, msg->src_ep, msg->src);
	case SM_SESSION_PACKET_CLOSE_ACK:
		session->remote_ep = 0;
		session->flags = 0;
	case SM_PACKET_READY:
		if (session->handle)
			session->handle(msg, session);
		sm_send_packet_ack(session, msg->src_ep, msg->src, msg->payload, msg->length);
		return ret;
	case SM_SESSION_PACKET_READY:
		if (session->handle)
			session->handle(msg, session);
		sm_send_session_packet_ack(session, msg->src_ep, msg->src, msg->payload, msg->length);
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

void register_sm_proto()
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
	sm_uint32_t slot;
	msg = &inqueue->messages[(received % SM_MSGQ_LEN)];

	slot = sm_find_session(msg->dst_ep, 0, coreb_info.icc_info.sessions_table);

	if (slot >=0 && slot < 32) {
		session = &coreb_info.icc_info.sessions_table[slot];
		session->proto_ops->recvmsg(msg, session);
	} else {
		sm_message_dequeue(cpu, msg);
		/* to do send error ack to cpu 0 */
	}
}

int default_session_handle(struct sm_message *msg, struct sm_session *session)
{
	struct sm_message tmp;
	coreb_msg(" %s session %d msg %s \n",__func__, session->local_ep, msg->payload);
	coreb_msg("dst %d dstep %d, src %d, srcep %d\n", msg->dst, msg->dst_ep, msg->src, msg->src_ep);
	sm_recv_message(msg, &tmp, session);

	/* handle payload */

	coreb_msg("msg payload %s \n", tmp.payload);
	return 0;
}

void testcase_session_handler()
{
	sm_uint32_t slot;
	struct sm_session *session;
	slot = sm_create_session(5, SP_PACKET, coreb_info.icc_info.sessions_table);
	if (slot >= 32)
		coreb_msg("create session failed\n");

	session = &coreb_info.icc_info.sessions_table[slot];
	sm_registe_session_handler(session, default_session_handle);
}
