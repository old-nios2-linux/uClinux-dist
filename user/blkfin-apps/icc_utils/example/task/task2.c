#include <icc.h>
#include <protocol.h>

#define LOCAL_SESSION 6
int default_session_handle(struct sm_message *message, struct sm_session *session);

sm_uint32_t __icc_task_data index;
void  __icc_task icc_task_init(int argc, char *argv[])
{
	struct sm_session *session;
	index = sm_create_session(LOCAL_SESSION, SP_SESSION_PACKET);
	coreb_msg("%s() %s %s index %d\n", __func__, argv[0], argv[1], index);
	if (index >= 32)
		coreb_msg("create session failed\n");

	session = &coreb_info.icc_info.sessions_table[index];
	sm_registe_session_handler(session, default_session_handle);
	coreb_msg("%s() end\n", __func__);
}

void  __icc_task icc_task_exit(void)
{
	sm_destroy_session(LOCAL_SESSION);
}

int  __icc_task default_session_handle(struct sm_message *message, struct sm_session *session)
{
	void *buf;
	struct l3_proto_head *p;
	sm_uint32_t len;
	int ret;
	struct sm_msg *msg = &message->msg;
	coreb_msg(" %s session %d msg %s \n",__func__, session->local_ep, msg->payload);
	coreb_msg("dst %d dstep %d, src %d, srcep %d\n", message->dst, msg->dst_ep, message->src, msg->src_ep);

	ret = sm_recv_packet(index, &buf, len);
	if (ret <= 0) {
		coreb_msg("recv packet failed\n");
		return ret;
	}
	/* handle payload */
	coreb_msg("processing msg %s\n", buf);
	p = (struct l3_proto_head *)buf;
	if (p->type == L3_TYPE_AUDIO) {
		int len = sizeof(struct l3_proto_head);
		unsigned int audio_buf = p->chunk_addr;
		unsigned int size = p->chunk_size;
		int dst_ep = msg->src_ep;
		int dst_cpu = message->src;
		void *send_buf = sm_send_request(len, session);
		coreb_msg("coreb send buf %x\n", send_buf);
		if (!send_buf)
			coreb_msg("NO MEM\n");
		memset(send_buf, 0, len);
		p = send_buf;
		p->type = L3_TYPE_AUDIO;
		p->chunk_addr = audio_buf;
		p->chunk_size = size;
		p->status = 1;
		sm_send_packet(index, dst_ep, dst_cpu, send_buf, len);
	} else {
		coreb_msg("msg payload %s \n", buf);
	}

	sm_recv_release(buf, len, session);

	return 0;
}

