/*
 * dsp_sys.h
 *
 * Copyright 2009 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#ifndef __DSP_H
#define __DSP_H

#include <linux/init.h>
#include <linux/linkage.h>
#include "defs.h"

#ifdef CONFIG_COREB_LED_INDICATOR
#define LED13 GPIO_PF32
#define LED14 GPIO_PF33
#define LED15 GPIO_PF34
#define LED16 GPIO_PF35
#define LED17 GPIO_PF36 /* GPIO bank 2, bitmask 0x10, indicate trap occured */
#define LED18 GPIO_PF37 /* GPIO bank 2, bitmask 0x20, indicate hw error */
#define LED19 GPIO_PF38 /* GPIO bank 2, bitmask 0x40, indicate interrurt */
#define LED20 GPIO_PF39 /* GPIO band 2, bitmask 0x80, flicker on core timer */

#define LED5  GPIO_PF40 /* IPI1 */
#define LED6  GPIO_PF41
#define LED7  GPIO_PF42
#define LED8  GPIO_PF43
#define LED9  GPIO_PF44
#define LED10 GPIO_PF45
#define LED11 GPIO_PF46
#define LED12 GPIO_PF47

#endif

/* #define __init __section(.text) __cold notrace */

#ifdef __ASSEMBLY__
# ifdef CONFIG_COREB_LED_INDICATOR
/* LED17 GPIO_PF36 */
#  define TRAP_LED_SADD  0xFFC01708
#  define TRAP_LED_CADD  0xFFC01704
#  define TRAP_LED_BIT   0x10
/* LED18 GPIO_PF37 */
#  define HWER_LED_SADD  0xFFC01708
#  define HWER_LED_CADD  0xFFC01704
#  define HWER_LED_BIT   0x20
/* LED19 GPIO_PF38 */
#  define INTR_LED_SADD  0xFFC01708
#  define INTR_LED_CADD  0xFFC01704
#  define INTR_LED_BIT   0x40

/* assume we can touch stack */
#  define LED_FLICKER(gpioreg, gpiobit)	\
	[--sp] = r1;			\
	[--sp] = r0;			\
	[--sp] = p0;			\
	p0.l = gpioreg;			\
	p0.h = gpioreg;			\
	r1 = [p0];			\
	p0.l = gpiobit;			\
	p0.h = gpiobit;			\
	r0 = [p0];			\
	p0 = r1;			\
	w[p0] = r0;			\
	p0 = [sp++];			\
	r0 = [sp++];			\
	r1 = [sp++];

# else
#  define LED_FLICKER(gpioreg, gpiobit)

# endif

#else
#include <linux/irq.h>
#include <linux/irqnr.h>
#include <linux/interrupt.h>

extern unsigned long bfin_irq_flags;

struct ivgx {
	unsigned int irqno;
	unsigned int isrflag;
};

struct ivg_slice {
	struct ivgx *ifirst;
	struct ivgx *istop;
};

void __init dsp_init(void) __attribute__ ((longcall));
void dsp_panic(unsigned long err);
void dsp_stub(void);

void evt_nmi(void);
void trap(void);
void evt_ivhw(void);
void evt_timer(void);
void evt_evt7(void);
void evt_evt8(void);
void evt_evt9(void);
void evt_evt10(void);
void evt_evt11(void);
void evt_evt12(void);
void evt_evt13(void);
void evt_evt14(void);
void evt_evt15(void);

#define irqs_save_flags(flags) do { (flags) = bfin_read_IMASK(); } while (0)
#define irqs_is_disabled_flags(flags) (((flags) & ~0x3f) == 0)
#define irqs_is_disabled()			\
({						\
         unsigned long _flags;			\
         irqs_save_flags(_flags);		\
         irqs_is_disabled_flags(_flags);	\
})

static inline unsigned long irq_save(void)
{
	unsigned long flags = bfin_cli();
	return flags;
}

static inline void irq_restore(unsigned long flags)
{
	bfin_sti(flags);
}

static inline void irq_enable()
{
	bfin_sti(bfin_irq_flags);
}

static inline void irq_disable()
{
	bfin_cli();
}
#define irq_enable_in_hardirq()  irq_enable()

#endif
#endif
