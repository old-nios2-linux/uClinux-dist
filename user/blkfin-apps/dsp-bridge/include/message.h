/*
 * message.h
 *
 * Copyright 2009 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#ifndef __MESSAGE_H
#define __MESSAGE_H

#define MSG_NUM		16
#define MSG_SIZE	128	/* bytes */

#if defined(__DSP__) || defined(__KERNEL__)
#include <linux/list.h>

#endif

enum msg_type {
	MSG_ACK_OK = 0,
	MSG_ACK_FAIL,
	MSG_DSP_START,		/* make dsp start user program */
	MSG_DSP_RESET,		/* make dsp waiting for restart user program */
	MSG_DSP_STOP,		/* currently HW not support */
	MSG_DSP_RBF_STS,	/* notice the dsp that the ringbuf status */
	MSG_KERN_PRINT,		/* print messages from dsp */
	MSG_KERN_RBF_WP,	/* wake the task wait on ringbuf */
	MSG_KERN_RES_RESERVE,	/* resource reserve */
	MSG_KERN_RES_RELEASE,	/* resource release */
	MSG_USER_DEF = 100,
};

struct dsp_list_head {
	struct dsp_list_head *next, *prev;
};

struct message {
#if defined(__DSP__) || defined(__KERNEL__)
	struct list_head list;
#else
	struct dsp_list_head list;
#endif
	unsigned long index;
	unsigned long type;
	unsigned long pid;
	void *data; /* data pointer */
	unsigned long size;  /* size in byte */
};

struct msg_t {
};

/* MSG_ACK_OK MSG_ACK_FAIL MSG_DSP_RESET MSG_DSP_STOP */
struct msg_quick_t {
	int code;
};

/* MSG_KERN_PRINT */
struct msg_data_t {
	unsigned char data[MSG_SIZE];
};

/* MSG_KERN_RES_RESERVE MSG_KERN_RES_RELEASE */
struct msg_resource_t {
	unsigned long res_id;
	long begin;
	long end;
};

/* MSG_KERN_RES_RESERVE MSG_KERN_RES_RELEASE */
struct msg_resourcelist_t {
	unsigned long res_id;
	unsigned long num;
	long list[MSG_SIZE/sizeof(long) - 2];
};

union msg_buf {
	struct msg_t msg;
	struct msg_quick_t msgq;
	struct msg_data_t msgd;
	struct msg_resource_t msgr;
	struct msg_resourcelist_t msgrl;
	long placeholder[MSG_SIZE/sizeof(long)];
};

struct message_ch {
	unsigned long lock;
#if defined(__DSP__) || defined(__KERNEL__)
	struct list_head head;
#else
	struct dsp_list_head head;
#endif
	unsigned long map;
	struct message m_slot[MSG_NUM];
};

#if defined(__DSP__) || defined(__KERNEL__)
struct message * get_message_slot(void);

void put_message_slot(unsigned long);

void put_receive_message_slot(unsigned long);

int message_init(int (* dispatch_message)(struct message *));
void send_message(struct message *);
unsigned long mk_data_message(struct message *, unsigned long, unsigned char *, long);
#endif

#endif
