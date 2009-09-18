/*
 * sha.h
 *
 * Copyright 2009 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#ifndef __NONCACHED_H
#define __NONCACHED_H

#include "atomic.h"
#include "message.h"
#include "ringbuf.h"

#define NONCACHED (void *)0x02400000

static struct sha_struct {
#ifdef __DSP__
	struct message_ch send_msg; /* dsp send channel */
	struct message_ch receive_msg; /* dsp receive channel */
	union msg_buf send_buf[MSG_NUM];
	union msg_buf receive_buf[MSG_NUM];

	struct ringbuf_ch write_rbf[RBF_PAIR]; 
	struct ringbuf_ch read_rbf[RBF_PAIR];
	unsigned char write_buf[RBF_PAIR][RBF_SIZE];
	unsigned char read_buf[RBF_PAIR][RBF_SIZE];
#else

	struct message_ch receive_msg; /* kernel receive channel */
	struct message_ch send_msg; /* kernel send channel */
	union msg_buf receive_buf[MSG_NUM];
	union msg_buf send_buf[MSG_NUM];

	struct ringbuf_ch read_rbf[RBF_PAIR];
	struct ringbuf_ch write_rbf[RBF_PAIR]; 
	unsigned char read_buf[RBF_PAIR][RBF_SIZE];
	unsigned char write_buf[RBF_PAIR][RBF_SIZE];
#endif
	unsigned long dsp_start;
	unsigned long dsp_reset;
	unsigned long dsp_stub_addr;
} *sha = NONCACHED;

#endif
