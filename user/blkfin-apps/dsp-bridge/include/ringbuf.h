/*
 * ringbuf.h
 *
 * Copyright 2009 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#ifndef __RINGBUF_H
#define __RINGBUF_H

#define RBF_PAIR	1
#define RBF_SIZE	1024	/* bytes */

#include <linux/wait.h>

struct dsp_wait_queue_head_t {
	/* unsigned long lock; */
	struct list_head task_list;
};

struct ringbuf_ch {
#ifdef __DSP__
	struct dsp_wait_queue_head_t queue;
#else
	wait_queue_head_t queue;
#endif
	ssize_t reader;
	ssize_t writer;
	void *data;
	ssize_t size;
	unsigned long rlock;
	unsigned long wlock;
	unsigned long wakeup;
};

int ringbuf_init(void);

void ringbuf_flush(struct ringbuf_ch *);
void ringbuf_reset(struct ringbuf_ch *);
#ifdef __DSP__
int ringbuf_wakeup(struct ringbuf_ch *);
ssize_t rbf_read(unsigned long, unsigned long, char *, size_t);
ssize_t rbf_write(unsigned long, unsigned long, const char *, size_t);
#else
ssize_t ringbuf_read_user(struct ringbuf_ch *, char __user *, size_t);
ssize_t ringbuf_write_user(struct ringbuf_ch *, const char __user *, size_t);
ssize_t rbf_read_user(unsigned long, unsigned long, char __user *, size_t);
ssize_t rbf_write_user(unsigned long, unsigned long, const char __user *, size_t);
void ringbuf_flush_user(unsigned long);
void ringbuf_reset_user(unsigned long);
#endif

#endif
