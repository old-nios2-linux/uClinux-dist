/*
 * CoreB side lowlevel assembly code.
 *
 * Based on arch/blackfin/mach-common/entry.S
 *
 * Copyright 2009 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/init.h>
#include <linux/linkage.h>
#include <asm/blackfin.h>
#include <asm/context.S>
#include <asm/entry.h>
#include "dsp_sys.h"

.section .l1.text
p0.h = __start;
p0.l = __start;
jump (p0);

.section .text
ENTRY(_evt_nmi)
.nmi:	ssync;
	jump .nmi;
ENDPROC(_evt_nmi)

ENTRY(_trap)
	LED_FLICKER(_trap_led_set_reg, _trap_led_bit)
.trap:	ssync;
	jump .trap;
ENDPROC(_trap)

ENTRY(_evt_ivhw)
	ssync;
	ssync;
	LED_FLICKER(_hwer_led_set_reg, _hwer_led_bit)
.ivhw:	ssync;
	jump .ivhw;
ENDPROC(_evt_ivhw)

.align 4
__common_int_entry:
	LED_FLICKER(_intr_led_set_reg, _intr_led_bit)

	[--sp] = fp;
	[--sp] = usp;

	[--sp] = i0;
	[--sp] = i1;
	[--sp] = i2;
	[--sp] = i3;

	[--sp] = m0;
	[--sp] = m1;
	[--sp] = m2;
	[--sp] = m3;

	[--sp] = l0;
	[--sp] = l1;
	[--sp] = l2;
	[--sp] = l3;

	[--sp] = b0;
	[--sp] = b1;
	[--sp] = b2;
	[--sp] = b3;
	[--sp] = a0.x;
	[--sp] = a0.w;
	[--sp] = a1.x;
	[--sp] = a1.w;

	[--sp] = LC0;
	[--sp] = LC1;
	[--sp] = LT0;
	[--sp] = LT1;
	[--sp] = LB0;
	[--sp] = LB1;

	[--sp] = ASTAT;

	[--sp] = r0;	/* Skip reserved */
	[--sp] = RETS;
	r2 = RETI;
	[--sp] = r2;
	[--sp] = RETX;
	[--sp] = RETN;
	[--sp] = RETE;
	[--sp] = SEQSTAT;
	[--sp] = r1;	/* IPEND - R1 may or may not be set up before jumping here. */

	cli r1;
	[--sp] = RETI;  /* orig_pc */
	/* Clear all L registers.  */
	r1 = 0 (x);
	l0 = r1;
	l1 = r1;
	l2 = r1;
	l3 = r1;
#ifdef CONFIG_FRAME_POINTER
	fp = 0;
#endif

#if ANOMALY_05000283 || ANOMALY_05000315
	cc = r7 == r7;
	p5.h = HI(CHIPID);
	p5.l = LO(CHIPID);
	if cc jump 1f;
	r7.l = W[p5];
1:
#endif
	r1 =  sp;
	SP += -12;
	call _do_irq;
	SP += 12;

	RESTORE_CONTEXT

	LED_FLICKER(_intr_led_clr_reg, _intr_led_bit)
	rti;

ENTRY(_evt_timer)
	INTERRUPT_ENTRY(EVT_IVTMR_P)
ENTRY(_evt_evt7)
	INTERRUPT_ENTRY(EVT_IVG7_P)
ENTRY(_evt_evt8)
	INTERRUPT_ENTRY(EVT_IVG8_P)
ENTRY(_evt_evt9)
	INTERRUPT_ENTRY(EVT_IVG9_P)
ENTRY(_evt_evt10)
	INTERRUPT_ENTRY(EVT_IVG10_P)
ENTRY(_evt_evt11)
	INTERRUPT_ENTRY(EVT_IVG11_P)
ENTRY(_evt_evt12)
	INTERRUPT_ENTRY(EVT_IVG12_P)
ENTRY(_evt_evt13)
	INTERRUPT_ENTRY(EVT_IVG13_P)
ENTRY(_evt_evt14)
	INTERRUPT_ENTRY(EVT_IVG14_P)
ENTRY(_evt_evt15)
	INTERRUPT_ENTRY(EVT_IVG15_P)

