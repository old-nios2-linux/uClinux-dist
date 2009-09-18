/*
 * CoreB system function.
 *
 * Based on blackfin specific kernel code.
 *
 * Copyright 2009 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/irq.h>
#include <linux/irqnr.h>
#include <linux/interrupt.h>
#include "dsp_sys.h"
#include "sha.h"

atomic_t num_spurious;

extern struct ivg_slice ivg7_13[];

struct irq_desc irq_desc[NR_IRQS] = {
	[0 ... NR_IRQS-1] = {
		.status = IRQ_DISABLED,
		.depth = 1,
		.action = NULL,
	}
};

struct irq_desc *irq_to_desc(unsigned int irq)
{
	return (irq < NR_IRQS) ? irq_desc + irq : NULL;
}

irqreturn_t handle_IRQ_event(unsigned int irq, struct irqaction *action)
{
	irqreturn_t ret, retval = IRQ_NONE;
	unsigned int status = 0;

	ret = action->handler(irq, action->dev_id);
	if (ret == IRQ_HANDLED)
		status |= action->flags;
	retval |= ret;

	return retval;
}

static int
__setup_irq(unsigned int irq, struct irq_desc * desc, struct irqaction *new)
{
	struct irqaction *old, **p;

	if (!desc)
		return -EINVAL;

	p = &desc->action;
	old = *p;
	if (old) {
		dsp_panic(PANIC_IRQ_ACTION_EXISTED);
	}

		desc->status &= ~(IRQ_AUTODETECT | IRQ_WAITING |
				  IRQ_INPROGRESS | IRQ_SPURIOUS_DISABLED);

		if (!(desc->status & IRQ_NOAUTOEN)) {
			desc->depth = 0;
			desc->status &= ~IRQ_DISABLED;
			desc->chip->startup(irq);
		} else
			/* Undo nested disables: */
			desc->depth = 1;

		/* Exclude IRQ from balancing if requested */
		if (new->flags & IRQF_NOBALANCING)
			desc->status |= IRQ_NO_BALANCING;

	*p = new;

	/* Reset broken irq detection when installing new handler */
	desc->irq_count = 0;
	desc->irqs_unhandled = 0;


	new->irq = irq;
	return 0;
}

int setup_irq(unsigned int irq, struct irqaction *act)
{
	struct irq_desc *desc = irq_to_desc(irq);

	return __setup_irq(irq, desc, act);
}


void handle_simple_irq(unsigned int irq, struct irq_desc *desc)
{
	struct irqaction *action;
	irqreturn_t action_ret;

	if (unlikely(desc->status & IRQ_INPROGRESS))
		return;
	desc->status &= ~(IRQ_REPLAY | IRQ_WAITING);

	action = desc->action;
	if (unlikely(!action || (desc->status & IRQ_DISABLED)))
		return;

	desc->status |= IRQ_INPROGRESS;

	action_ret = handle_IRQ_event(irq, action);

	desc->status &= ~IRQ_INPROGRESS;
}

void do_irq(int vec, struct pt_regs *fp)
{
	struct irq_desc *desc;

	if (vec == EVT_IVTMR_P) {
		vec = IRQ_CORETMR;
	} else {
		struct ivgx *ivg = ivg7_13[vec - IVG7].ifirst;
		struct ivgx *ivg_stop = ivg7_13[vec - IVG7].istop;
#ifdef SICB_ISR0
		unsigned long sic_status[3];
		sic_status[0] = bfin_read_SICB_ISR0() & bfin_read_SICB_IMASK0();
		sic_status[1] = bfin_read_SICB_ISR1() & bfin_read_SICB_IMASK1();
# ifdef SICB_ISR2
		sic_status[2] = bfin_read_SIC_ISR2() & bfin_read_SIC_IMASK2();
# endif
		for (;; ivg++) {
			if (ivg >= ivg_stop) {
				atomic_inc(&num_spurious);
				return;
			}
			if (sic_status[(ivg->irqno - IVG7) / 32] & ivg->isrflag)
				break;
		}
#else
		unsigned long sic_status;

		sic_status = bfin_read_SIC_IMASK() & bfin_read_SIC_ISR();

		for (;; ivg++) {
			if (ivg >= ivg_stop) {
				atomic_inc(&num_spurious);
				return;
			} else if (sic_status & ivg->isrflag)
				break;
		}
#endif
		vec = ivg->irqno;
	}
	/*asm_do_IRQ(vec, fp);*/
	desc = irq_to_desc(vec);
	desc->handle_irq(vec, desc);
}

void dsp_panic(unsigned long err)
{
	static volatile unsigned long panic_code;
	while (1)
	{
		panic_code = err;
	}
}

static u_long get_clkin_hz(void)
{
	        return CONFIG_CLKIN_HZ;
}
static u_long get_vco(void)
{
	static u_long cached_vco;
	u_long msel, pll_ctl;

	if (cached_vco)
		return cached_vco;

	pll_ctl = bfin_read_PLL_CTL();
	msel = (pll_ctl >> 9) & 0x3F;
	if (0 == msel)
		msel = 64;

	cached_vco = get_clkin_hz();
	cached_vco >>= (1 & pll_ctl);	/* DF bit */
	cached_vco *= msel;
	return cached_vco;
}

u_long get_cclk(void)
{
	static u_long cached_cclk_pll_div, cached_cclk;
	u_long csel, ssel;

	if (bfin_read_PLL_STAT() & 0x1)
		return get_clkin_hz();

	ssel = bfin_read_PLL_DIV();
	if (ssel == cached_cclk_pll_div)
		return cached_cclk;
	else
		cached_cclk_pll_div = ssel;

	csel = ((ssel >> 4) & 0x03);
	ssel &= 0xf;
	if (ssel && ssel < (1 << csel))	/* SCLK > CCLK */
		cached_cclk = get_vco() / ssel;
	else
		cached_cclk = get_vco() >> csel;
	return cached_cclk;
}

u_long get_sclk(void)
{
	static u_long cached_sclk;
	u_long ssel;

	if (cached_sclk)
		return cached_sclk;

	if (bfin_read_PLL_STAT() & 0x1)
		return get_clkin_hz();

	ssel = bfin_read_PLL_DIV() & 0xf;
	if (0 == ssel) {
		ssel = 1;
	}

	cached_sclk = get_vco() / ssel;
	return cached_sclk;
}

void dsp_deinit()
{
	bfin_write_SICB_IMASK0(SIC_UNMASK_ALL);
	bfin_write_SICB_IMASK1(SIC_UNMASK_ALL);
	irq_disable();
}

