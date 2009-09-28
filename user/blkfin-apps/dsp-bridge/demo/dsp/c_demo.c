/*
 * dsp side demo application.
 *
 * Copyright 2009 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/init.h>
#include <linux/jiffies.h>
#include "defs.h"
#include "sha.h"
#include "dsp_sys.h"
#include "message.h"
#include "ringbuf.h"
#include "demo_defs.h"

#define BLEN 300
/* print out some messages */
int msg_num = 0;
/* send to which user application */
int user_pid = 0;

int my_dispatch_message(struct message *msg)
{
	struct message *new_msg;
	struct msg_quick_t *msgq;

	switch (msg->type) {
	case MSG_USER_1:
		msgq = (struct msg_quick_t *)msg->data;
		msg_num = (int)msgq->code;
		user_pid = msg->pid;

		new_msg = get_message_slot();
		if (!new_msg) return 0;
		new_msg->size = snprintf((char *)new_msg->data, MSG_SIZE,
			"received MSG_USER_1 from pid %d, setting msg_num=%d\n",
							user_pid, msg_num) + 1;
		if (mk_data_message(new_msg, MSG_KERN_PRINT, NULL, new_msg->size))
			send_message(new_msg);
		break;
	case MSG_USER_2:
		break;
	default:
		break;
	}

	return 0;
}

int main()
{
	int len, cur, todo = 0;
	struct message *msg;
	static unsigned long jiff = 0;
	char buf[BLEN];

	dsp_init();
	message_init(my_dispatch_message);
	ringbuf_init();
stub:
	dsp_stub();

	msg = get_message_slot();
	msg->size = snprintf((char *)msg->data, MSG_SIZE, \
			"CoreB starting ...\n") + 1;
	if (mk_data_message(msg, MSG_KERN_PRINT, NULL, msg->size))
		send_message(msg);


	/* Do what you want */
	while (1) {
		if (!todo) {
			todo = rbf_read(0, 1, buf, BLEN);
			cur = 0;
		}
		len = rbf_write(0, 1, buf + cur, todo);
		cur += len;
		todo -= len;

		if (time_after(jiffies, jiff) && msg_num-- > 0) {
			jiff = (unsigned long)jiffies + 50;
			msg = get_message_slot();
			if (!msg) continue;
			msg->size = snprintf((char *)msg->data, MSG_SIZE,
				"message index %02d, use slot %02d\n",
					msg_num + 1, (int)msg->index) + 1;
			msg->pid = user_pid;
			if (mk_data_message(msg, MSG_USER_1, NULL, msg->size))
				send_message(msg);
		}

		if (asm_read(&sha->dsp_reset)) {
			asm_set(&sha->dsp_reset, 0);
			goto stub;
		}
	}
	return 0;
}
