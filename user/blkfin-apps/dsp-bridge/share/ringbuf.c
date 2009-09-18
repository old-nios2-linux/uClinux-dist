/*
 * File:		ringbuf.c
 * Based on:		drivers/media/dvb/dvb-core/dvb_ringbuffer.c
 * Author:		Graff Yang
 *
 * Copyright 2009 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#ifdef __DSP__
#else
# include <asm/uaccess.h>
# include <asm/string.h>
#endif

#include "defs.h"
#include "sha.h"
#include "ringbuf.h"
#include "dsp_sys.h"

void ringbuf_init_one(struct ringbuf_ch *rbuf, void *data, unsigned long size)
{
	rbuf->reader = rbuf->writer = 0;
	rbuf->data = data;
	rbuf->size = size;
	rbuf->rlock = 0;
	rbuf->wlock = 0;
	asm_set(&rbuf->wakeup, 0);
}

int ringbuf_empty(struct ringbuf_ch *rbuf)
{
	return (rbuf->reader == rbuf->writer);
}

ssize_t ringbuf_free(struct ringbuf_ch *rbuf)
{
	ssize_t free;
	free = rbuf->reader - rbuf->writer;
	if (free <= 0)
		free += rbuf->size;

	return (unsigned long)(free - 1);
}

ssize_t ringbuf_avail(struct ringbuf_ch *rbuf)
{
	ssize_t avail;
	avail = rbuf->writer - rbuf->reader;
	if (avail < 0)
		avail += rbuf->size;
	return (unsigned long)avail;
}

void ringbuf_flush(struct ringbuf_ch *rbuf)
{
	rbuf->reader = rbuf->writer;
}

void ringbuf_reset(struct ringbuf_ch *rbuf)
{
	rbuf->reader = rbuf->writer = 0;
}

ssize_t ringbuf_read(struct ringbuf_ch *rbuf, char *buf, size_t len)
{
	size_t todo = len;
	size_t split;

	split = (rbuf->reader + len > rbuf->size) ? rbuf->size - rbuf->reader : 0;
	if (split > 0) {
		memcpy(buf, rbuf->data + rbuf->reader, split);
		buf += split;
		todo -= split;
		rbuf->reader = 0;
	}
	memcpy(buf, rbuf->data + rbuf->reader, todo);

	rbuf->reader = (rbuf->reader + todo);
	if (rbuf->reader >= rbuf->size) 
		rbuf->reader = 0;

	return len;
}

static unsigned long orig_writer;
ssize_t ringbuf_write(struct ringbuf_ch *rbuf, const char *buf, size_t len)
{
	size_t todo = len;
	size_t split;

	orig_writer = rbuf->writer;
	split = (rbuf->writer + len > rbuf->size) ? rbuf->size - rbuf->writer : 0;
	if (split > 0) {
		memcpy(rbuf->data + rbuf->writer, buf, split);
		buf += split;
		todo -= split;
		rbuf->writer = 0;
	}
	memcpy(rbuf->data + rbuf->writer, buf, todo);
	rbuf->writer = (rbuf->writer + todo);
	if (rbuf->writer >= rbuf->size)
		rbuf->writer = 0;

	return len;
}

#ifdef __DSP__
int ringbuf_wakeup(struct ringbuf_ch *rbuf)
{
	struct message *msg;
	struct msg_quick_t *msgq;

	asm_set(&rbuf->wakeup, 0);

	msg = get_message_slot();
	msg->type = MSG_KERN_RBF_WP;
	msgq = (struct msg_quick_t *)msg->data;
	msgq->code = (int)rbuf;
	send_message(msg);
	return 0;
}

ssize_t rbf_read(unsigned long idx, unsigned long nonblock, char *buf, size_t len)
{
	ssize_t avail;
	ssize_t todo;
	ssize_t read = 0;
	struct ringbuf_ch *rbuf;
	int ret;

	if (!len) return 0;
	if (idx >= RBF_PAIR) return 0;

	rbuf = &sha->read_rbf[idx];

	lock(&rbuf->rlock);
	do {
		todo = len - read;
		avail = ringbuf_avail(rbuf);
		if (avail == 0 && asm_read(&rbuf->wakeup))
			ringbuf_wakeup(rbuf);

		todo = (avail > todo) ? todo : avail;

		ret = ringbuf_read(rbuf, buf, todo);
		if (ret < 0) break;

		buf += ret;
		read += ret;
	} while (!nonblock && read < len);
	unlock(&rbuf->rlock);

	return read;
}

ssize_t rbf_write(unsigned long idx, unsigned long nonblock, const char *buf, size_t len)
{
	ssize_t free;
	ssize_t todo;
	ssize_t wrote = 0;
	struct ringbuf_ch *rbuf;
	int ret;

	if (!len) return 0;
	if (idx >= RBF_PAIR) return 0;

	rbuf = &sha->write_rbf[idx];

	lock(&rbuf->wlock);
	do {
		todo = len - wrote;
		free = ringbuf_free(rbuf);
		if (free == 0 && asm_read(&rbuf->wakeup))
			ringbuf_wakeup(rbuf);

		todo = (free > todo) ? todo : free;

		ret = ringbuf_write(rbuf, buf, todo);
		if (ret < 0) break;

		buf += ret;
		wrote += ret;
	} while (!nonblock && wrote < len);
	unlock(&rbuf->wlock);

	if (ringbuf_avail(rbuf) && asm_read(&rbuf->wakeup))
		ringbuf_wakeup(rbuf);

	return wrote;
}
#else
ssize_t ringbuf_read_user(struct ringbuf_ch *rbuf, char __user *buf, size_t len)
{
	size_t todo = len;
	size_t split;

	split = (rbuf->reader + len > rbuf->size) ? rbuf->size - rbuf->reader : 0;
	if (split > 0) {
		if (copy_to_user(buf, rbuf->data + rbuf->reader, split))
			return -EFAULT;
		buf += split;
		todo -= split;
		rbuf->reader = 0;
	}
	if (copy_to_user(buf, rbuf->data + rbuf->reader, todo))
		return -EFAULT;
	rbuf->reader = (rbuf->reader + todo) % rbuf->size;

	return len;
}

ssize_t ringbuf_write_user(struct ringbuf_ch *rbuf, const char __user *buf, size_t len)
{
	size_t todo = len;
	size_t split;

	split = (rbuf->writer + len > rbuf->size) ? rbuf->size - rbuf->writer : 0;
	if (split > 0) {
		if (copy_from_user(rbuf->data + rbuf->writer, buf, split))
			return -EFAULT;
		buf += split;
		todo -= split;
		rbuf->writer = 0;
	}
	if (copy_from_user(rbuf->data + rbuf->writer, buf, todo))
			return -EFAULT;
	rbuf->writer = (rbuf->writer + todo) % rbuf->size;

	return len;
}

void ringbuf_flush_user(unsigned long idx)
{
	struct ringbuf_ch *rbuf;

	rbuf = &sha->read_rbf[idx];
	ringbuf_flush(rbuf);

	rbuf = &sha->write_rbf[idx];
	ringbuf_flush(rbuf);
}

void ringbuf_reset_user(unsigned long idx)
{	
	struct ringbuf_ch *rbuf;

	rbuf = &sha->read_rbf[idx];
	ringbuf_reset(rbuf);

	rbuf = &sha->write_rbf[idx];
	ringbuf_reset(rbuf);
}

ssize_t rbf_read_user(unsigned long idx, unsigned long nonblock, char __user *buf, size_t len)
{
	ssize_t avail;
	ssize_t todo;
	ssize_t read = 0;
	struct ringbuf_ch *rbuf;
	int ret;

	if (!len) return 0;
	if (idx >= RBF_PAIR) return -EINVAL;

	rbuf = &sha->read_rbf[idx];

	lock(&rbuf->rlock);
	do {
		todo = len - read;
		avail = ringbuf_avail(rbuf);
		if (avail == 0) {
			pr_debug("%s: goto wait\n", __func__);
			asm_set(&rbuf->wakeup, 1);
			ret = wait_event_interruptible(rbuf->queue,
					(ringbuf_avail(rbuf) > 0));
		}

		todo = (avail > todo) ? todo : avail;

		ret = ringbuf_read_user(rbuf, buf, todo);
		if (ret < 0) break;

		buf += ret;
		read += ret;
	} while (!nonblock && read < len);
	unlock(&rbuf->rlock);

	return read;
}

ssize_t rbf_write_user(unsigned long idx, unsigned long nonblock, const char __user *buf, size_t len)
{
	ssize_t free;
	ssize_t todo;
	ssize_t wrote = 0;
	struct ringbuf_ch *rbuf;
	int ret;

	if (!len) return 0;
	if (idx >= RBF_PAIR) return -EINVAL;

	rbuf = &sha->write_rbf[idx];

	lock(&rbuf->wlock);
	do {
		todo = len - wrote;
		free = ringbuf_free(rbuf);
		if (free == 0) {
			pr_debug("%s: goto wait\n", __func__);
			asm_set(&rbuf->wakeup, 1);
			ret = wait_event_interruptible(rbuf->queue,
					(ringbuf_free(rbuf) > 0));
		}

		todo = (free > todo) ? todo : free;

		ret = ringbuf_write_user(rbuf, buf, todo);
		if (ret < 0) break;

		buf += ret;
		wrote += ret;
	} while (!nonblock && wrote < len);
	unlock(&rbuf->wlock);

	return wrote;
}
#endif

int ringbuf_init(void)
{
	int i;
	struct ringbuf_ch *rbuf;
	void *data;

	assert(sizeof(struct dsp_wait_queue_head_t) == \
			sizeof(wait_queue_head_t));

	for (i = 0; i < RBF_PAIR; i++) {
		rbuf = &sha->write_rbf[i];
		data = &sha->write_buf[i];
		ringbuf_init_one(rbuf, data, RBF_SIZE);
#ifdef __DSP__
		INIT_LIST_HEAD(&rbuf->queue.task_list);
#else
		pr_debug("%s: rbuf: %p data: %p\n", __func__, rbuf, data);
		init_waitqueue_head(&rbuf->queue);
#endif
	}
	return i;
}

