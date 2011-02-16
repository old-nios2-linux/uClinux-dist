#include <icc.h>
#include "protocol.h"

#define LOCAL_SESSION 5
int default_session_handle(struct sm_message *msg, struct sm_session *session);

sm_uint32_t index;
void icc_task_init(int argc, char *argv[])
{
	struct sm_session *session;
	index = sm_create_session(LOCAL_SESSION, SP_PACKET);
	coreb_msg("%s() %s %s index %d\n", __func__, argv[0], argv[1], index);
	if (index >= 32)
		coreb_msg("create session failed\n");

	session = &coreb_info.icc_info.sessions_table[index];
	sm_registe_session_handler(session, default_session_handle);
	coreb_msg("%s() end\n", __func__);
}

void icc_task_exit(void)
{
	sm_destroy_session(index);
}

int default_session_handle(struct sm_message *msg, struct sm_session *session)
{
	void *buf;
	sm_uint32_t len;
	int ret;
	coreb_msg(" %s session %d msg %s \n",__func__, session->local_ep, msg->payload);
	coreb_msg("dst %d dstep %d, src %d, srcep %d\n", msg->dst, msg->dst_ep, msg->src, msg->src_ep);

	ret = sm_recv_packet(index, &buf, len);
	if (ret <= 0) {
		coreb_msg("recv packet failed\n");
		return ret;
	}
	/* handle payload */
	coreb_msg("processing msg %s\n", buf);
	if (*(char *)buf == '1') {
		int len = 64;
		int dst_ep = msg->src_ep;
		int dst_cpu = msg->src;
		void *send_buf = sm_send_request(len, session);
		coreb_msg("coreb send buf %x\n", send_buf);
		if (!send_buf)
			coreb_msg("NO MEM\n");
		memset(send_buf, 0, len);
		*(char *)send_buf = 'f';
		sm_send_packet(index, dst_ep, dst_cpu, send_buf, len);
	} else {
		coreb_msg("msg payload %s \n", buf);
	}

	sm_recv_release(buf, len, session);

	return 0;
}


