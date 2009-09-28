/*
 * File:		message.c
 * Based on:
 * Author:		Graff Yang
 *
 * Copyright 2009 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/list.h>
#include <linux/irq.h>
#include <linux/irqnr.h>
#include <linux/interrupt.h>

#ifdef __DSP__
#else
# include <linux/device.h>
# include <linux/fs.h>
# include <linux/kernel.h>
# include <asm/uaccess.h>
# include <asm/gpio.h>
# include <asm/string.h>
#endif

#include <asm-generic/bitops/fls.h>

#include "defs.h"
#include "dsp_sys.h"
#include "message.h"
#include "atomic.h"
#include "sha.h"
#include "ipi.h"

static int (* user_dispatch_message)(struct message *);

struct message * get_message_slot(void)
{
	unsigned long i;
	unsigned long warn = 1;
#ifdef __DSP__
	unsigned long max_try = 500;
retry:
#endif
	i = fls(sha->send_msg.map);

	if (i == 0 || i > MSG_NUM) {
#ifdef __DSP__
		if (max_try--) {
			asm("ssync;\n");
			goto retry;
		}
		dsp_panic(PANIC_OUT_OF_MSG_SLOT);
#else
		if (warn) {
			printk(KERN_EMERG "Out of message slots,\
				there are problems in your applications!\n");
			warn = 0;
		}
		return NULL;
#endif
	}

	i -= 1;
	sha->send_msg.map &= ~(1 << i);

	if (i < (MSG_NUM / 4) && warn) {
#ifdef __DSP__
		struct message *msg;
		msg = &sha->send_msg.m_slot[i];
		/* snprintf return printed character number, not include the '\0' */
		msg->size = snprintf((char *)msg->data, MSG_SIZE,
			"Message slots insufficient, \
			you sending messages to kernel too fast\n") + 1;
		mk_data_message(msg, MSG_KERN_PRINT, NULL, msg->size);
		send_message(msg);
		warn = 0;
		goto retry;
#else
		printk(KERN_ALERT "Message slots insufficient!\n");
#endif
	}
	return &sha->send_msg.m_slot[i];
}

void put_message_slot(unsigned long index)
{
	sha->send_msg.map |= 1 << index;
	sha->send_msg.m_slot[index].data = \
		(unsigned char *)&sha->send_buf[index];
}

void put_receive_message_slot(unsigned long index)
{
	sha->receive_msg.map |= 1 << index;
	sha->receive_msg.m_slot[index].data = \
		(unsigned char *)&sha->receive_buf[index];
}

unsigned long mk_data_message(struct message *msg, unsigned long type, unsigned char *buf, long size)
{
	if (!msg) return 0;

	msg->type = type;

	if (size > MSG_SIZE) {
		if (buf) memcpy(msg->data, buf, MSG_SIZE);
		msg->size = MSG_SIZE;
		return MSG_SIZE;
	} else {
		if (buf) memcpy(msg->data, buf, size);
		msg->size = size;
		return size;
	}
}

void send_message(struct message *msg)
{
	lock(&sha->send_msg.lock);
	list_add_tail(&msg->list, &sha->send_msg.head);
	unlock(&sha->send_msg.lock);
	send_ipi0();
}

static int dispatch_message(struct message *msg)
{
	int ret = 0;
	struct msg_quick_t *msgq;
#ifndef __DSP__
	unsigned char *buf;
#endif

	switch (msg->type) {
	case MSG_ACK_OK:
		break;
	case MSG_ACK_FAIL:
#ifndef __DSP__
		msgq = (struct msg_quick_t *)msg->data;
		printk(KERN_ALERT "\nMSG_ACK_FAIL: %d\n", msgq->code);
#endif
		break;
#ifdef __DSP__
	case MSG_DSP_START:
		asm_set(&sha->dsp_start, 1);
		break;

	case MSG_DSP_STOP:
	case MSG_DSP_RESET:
		asm_set(&sha->dsp_start, 0);
		asm_set(&sha->dsp_reset, 1);
		break;
#else
	case MSG_KERN_PRINT:
		buf = msg->data;
		buf[msg->size - 1 ] = '\0';
		printk(KERN_ALERT "CoreB: %s", buf);
		break;
	case MSG_KERN_RBF_WP:
		msgq = (struct msg_quick_t *)msg->data;
		wake_up(&((struct ringbuf_ch *)msgq->code)->queue);
		pr_debug("dispatch_message: wake up ringbuf 0x%x\n",
							msgq->code);
		break;
#endif
	default:
		if (user_dispatch_message)
			(* user_dispatch_message)(msg);
	}
	put_receive_message_slot(msg->index);

	return ret;
}

static void receive_message(void)
{
	struct message *msg = NULL;

	lock(&sha->receive_msg.lock);
	while (!list_empty(&sha->receive_msg.head)) {
		msg = list_entry(sha->receive_msg.head.next, typeof(*msg), list);
		list_del(&msg->list);
		unlock(&sha->receive_msg.lock);

		dispatch_message(msg);

		lock(&sha->receive_msg.lock);
	}
	unlock(&sha->receive_msg.lock);
}

static irqreturn_t ipi0_handler(int irq, void *dev_instance)
{
	clear_ipi0();

	receive_message();

	return IRQ_HANDLED;
}

#ifdef __DSP__
static struct irqaction ipi0_irq_action = {
	.name		= "ipi",
	.flags		= IRQF_DISABLED, 
	.handler	= ipi0_handler,
	.dev_id		= NULL,
};
#endif

int message_init(int (* dispatch_message)(struct message *))
{
	int i;
	int ret = 0;
	struct message_ch *msg_ch;

	assert(sizeof(struct list_head) == sizeof(struct dsp_list_head));

	assert(MSG_NUM <= 32);

	msg_ch = &sha->send_msg;
	msg_ch->map = 0;
	for (i = 0; i < (MSG_NUM); i++) {
		msg_ch->map |= 1 << i;
		msg_ch->m_slot[i].index = i;
		msg_ch->m_slot[i].data = &sha->send_buf[i];
		INIT_LIST_HEAD(&sha->send_msg.m_slot[i].list);
	}

	INIT_LIST_HEAD(&sha->send_msg.head);
	sha->send_msg.lock = 0;


#ifdef __DSP__
	setup_irq(IRQ_SUPPLE_0, &ipi0_irq_action); 
#else
	ret = request_irq(IRQ_SUPPLE_0, ipi0_handler, IRQF_DISABLED, "ipi0", ipi0_handler);
#endif

	assert(MSG_SIZE >= 80);
	
	assert(sizeof(struct msg_quick_t) <= MSG_SIZE);
	assert(sizeof(struct msg_data_t) <= MSG_SIZE);
	assert(sizeof(struct msg_resource_t) <= MSG_SIZE);
	assert(sizeof(struct msg_resourcelist_t) <= MSG_SIZE);

	if (dispatch_message)
		user_dispatch_message = dispatch_message;

	return ret;
}
