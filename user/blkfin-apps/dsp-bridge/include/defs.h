/*
 * Global defines
 *
 * Copyright 2009 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#ifndef __DEF_H
#define __DEF_H

#define CONFIG_COREB_LED_INDICATOR

#ifndef __ASSEMBLY__
enum ioctl_cmd {
	CMD_COREB_START = 2,
	CMD_COREB_STOP,
	CMD_COREB_RESET,
	CMD_COREB_GET_MMREGION_NUM,
	CMD_COREB_GET_MMREGION,

	CMD_COREB_GET_MESSAGE_SLOT,
	CMD_COREB_PUT_RECEIVE_MESSAGE_SLOT,
	CMD_COREB_SEND_MESSAGE,
	CMD_COREB_SEND_MESSAGE_DIRECT,
	CMD_COREB_RECEIVE_MESSAGE,
	CMD_COREB_RECEIVE_MESSAGE_DIRECT,
};

enum panic_code {
	PANIC_IRQ_ACTION_EXISTED = 0xff000001,
	PANIC_OUT_OF_MSG_SLOT,
};
#endif

#ifdef __DSP__
#define assert(expr)

#else
#define assert(expr) do {					\
	if (unlikely(!(expr))) {				\
		printk(KERN_CRIT "assert failed in %s at %u\n",	\
				__func__, __LINE__);		\
	}							\
} while (0)
#endif

#endif
