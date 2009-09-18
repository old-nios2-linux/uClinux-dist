/*
 * CoreB initialize.
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
#include <linux/jiffies.h>
#include <mach/gpio.h>
#include <asm/gpio.h>
#include <asm/time.h>
#include <asm/trace.h>
#include "dsp_sys.h"
#include "ipi.h"
#include "sha.h"

#define SIC_SYSIRQ(irq) (irq - (IRQ_CORETMR + 1))

unsigned long volatile __jiffy_data jiffies;
unsigned long bfin_irq_flags = 0x1f;
struct ivgx ivg_table[NR_PERI_INTS];
struct ivg_slice ivg7_13[IVG13 - IVG7 + 1];

#ifdef CONFIG_COREB_LED_INDICATOR
unsigned int trap_led_set_reg;
unsigned int trap_led_clr_reg;
unsigned int hwer_led_set_reg; 
unsigned int hwer_led_clr_reg; 
unsigned int intr_led_set_reg;
unsigned int intr_led_clr_reg;
unsigned short trap_led_bit;
unsigned short hwer_led_bit;
unsigned short intr_led_bit;

bool ipi1_light = 0;
bool timer_light = 0;
extern struct gpio_port_t * const gpio_array[];

void __init coerb_led_indicator_init()
{
	trap_led_set_reg = /* 0xFFC01708 */
		(unsigned int)gpio_array[gpio_bank(LED17)] +
				offsetof(struct gpio_port_t, data_set);
	trap_led_clr_reg = /* 0xFFC01704 */
		(unsigned int )gpio_array[gpio_bank(LED17)] +
				offsetof(struct gpio_port_t, data_clear);
	hwer_led_set_reg =
		(unsigned int )gpio_array[gpio_bank(LED18)] +
				offsetof(struct gpio_port_t, data_set);
	hwer_led_clr_reg =
		(unsigned int )gpio_array[gpio_bank(LED18)] +
				offsetof(struct gpio_port_t, data_clear);
	intr_led_set_reg =
		(unsigned int )gpio_array[gpio_bank(LED19)] +
				offsetof(struct gpio_port_t, data_set);
	intr_led_clr_reg =
		(unsigned int )gpio_array[gpio_bank(LED19)] +
				offsetof(struct gpio_port_t, data_clear);
	trap_led_bit = gpio_bit(LED17);
	hwer_led_bit = gpio_bit(LED18);
	intr_led_bit = gpio_bit(LED19);

}
#endif

static void __init search_IAR(void)
{
	unsigned ivg, irq_pos = 0;
	for (ivg = 0; ivg <= IVG13 - IVG7; ivg++) {
		int irqn;

		ivg7_13[ivg].istop = ivg7_13[ivg].ifirst = &ivg_table[irq_pos];

		for (irqn = 0; irqn < NR_PERI_INTS; irqn++) {
			int iar_shift = (irqn & 7) * 4;
			if (ivg == (0xf &
			     bfin_read32((unsigned long *)SIC_IAR0 +
					(irqn >> 3)) >> iar_shift)) {
				ivg_table[irq_pos].irqno = IVG7 + irqn;
				ivg_table[irq_pos].isrflag = 1 << (irqn % 32);
				ivg7_13[ivg].istop++;
				irq_pos++;
			}
		}
	}
}

static void bfin_ack_noop(unsigned int irq)
{
}

static void bfin_core_mask_irq(unsigned int irq)
{
	bfin_irq_flags &= ~(1 << irq);
	if (!irqs_is_disabled())
		irq_enable();
}

static void bfin_core_unmask_irq(unsigned int irq)
{
	bfin_irq_flags |= 1 << irq;
	if (!irqs_is_disabled())
		irq_enable();
	return;
}

static void bfin_internal_mask_irq(unsigned int irq)
{
	unsigned long flags;

	unsigned mask_bank, mask_bit;
	flags = irq_save();
	mask_bank = SIC_SYSIRQ(irq) / 32;
	mask_bit = SIC_SYSIRQ(irq) % 32;
	bfin_write_SICB_IMASK(mask_bank, bfin_read_SICB_IMASK(mask_bank) &
			     ~(1 << mask_bit));
	irq_restore(flags);
}

static void bfin_internal_unmask_irq(unsigned int irq)
{
	unsigned long flags;

	unsigned mask_bank, mask_bit;
	flags = irq_save();
	mask_bank = SIC_SYSIRQ(irq) / 32;
	mask_bit = SIC_SYSIRQ(irq) % 32;
	bfin_write_SIC_IMASK(mask_bank, bfin_read_SIC_IMASK(mask_bank) |
			     (1 << mask_bit));
	bfin_write_SICB_IMASK(mask_bank, bfin_read_SICB_IMASK(mask_bank) |
			     (1 << mask_bit));
	irq_restore(flags);
}

static struct irq_chip bfin_core_irqchip = {
	.name = "CORE",
	.ack = bfin_ack_noop,
	.mask = bfin_core_mask_irq,
	.unmask = bfin_core_unmask_irq,
	.startup = bfin_core_unmask_irq,
};

static struct irq_chip bfin_internal_irqchip = {
	.name = "INTN",
	.ack = bfin_ack_noop,
	.mask = bfin_internal_mask_irq,
	.unmask = bfin_internal_unmask_irq,
	.mask_ack = bfin_internal_mask_irq,
	.disable = bfin_internal_mask_irq,
	.enable = bfin_internal_unmask_irq,
	.startup = bfin_internal_unmask_irq,
};

void __init program_IAR(void)
{
	/* Program the IAR0 Register with the configured priority */
	bfin_write_SICB_IAR0(((CONFIG_IRQ_PLL_WAKEUP - 7) << IRQ_PLL_WAKEUP_POS) |
			     ((CONFIG_IRQ_DMA1_ERROR - 7) << IRQ_DMA1_ERROR_POS) |
			     ((CONFIG_IRQ_DMA2_ERROR - 7) << IRQ_DMA2_ERROR_POS) |
			     ((CONFIG_IRQ_IMDMA_ERROR - 7) << IRQ_IMDMA_ERROR_POS) |
			     ((CONFIG_IRQ_PPI0_ERROR - 7) << IRQ_PPI0_ERROR_POS) |
			     ((CONFIG_IRQ_PPI1_ERROR - 7) << IRQ_PPI1_ERROR_POS) |
			     ((CONFIG_IRQ_SPORT0_ERROR - 7) << IRQ_SPORT0_ERROR_POS) |
			     ((CONFIG_IRQ_SPORT1_ERROR - 7) << IRQ_SPORT1_ERROR_POS));

	bfin_write_SICB_IAR1(((CONFIG_IRQ_SPI_ERROR - 7) << IRQ_SPI_ERROR_POS) |
			     ((CONFIG_IRQ_UART_ERROR - 7) << IRQ_UART_ERROR_POS) |
			     ((CONFIG_IRQ_RESERVED_ERROR - 7) << IRQ_RESERVED_ERROR_POS) |
			     ((CONFIG_IRQ_DMA1_0 - 7) << IRQ_DMA1_0_POS) |
			     ((CONFIG_IRQ_DMA1_1 - 7) << IRQ_DMA1_1_POS) |
			     ((CONFIG_IRQ_DMA1_2 - 7) << IRQ_DMA1_2_POS) |
			     ((CONFIG_IRQ_DMA1_3 - 7) << IRQ_DMA1_3_POS) |
			     ((CONFIG_IRQ_DMA1_4 - 7) << IRQ_DMA1_4_POS));

	bfin_write_SICB_IAR2(((CONFIG_IRQ_DMA1_5 - 7) << IRQ_DMA1_5_POS) |
			     ((CONFIG_IRQ_DMA1_6 - 7) << IRQ_DMA1_6_POS) |
			     ((CONFIG_IRQ_DMA1_7 - 7) << IRQ_DMA1_7_POS) |
			     ((CONFIG_IRQ_DMA1_8 - 7) << IRQ_DMA1_8_POS) |
			     ((CONFIG_IRQ_DMA1_9 - 7) << IRQ_DMA1_9_POS) |
			     ((CONFIG_IRQ_DMA1_10 - 7) << IRQ_DMA1_10_POS) |
			     ((CONFIG_IRQ_DMA1_11 - 7) << IRQ_DMA1_11_POS) |
			     ((CONFIG_IRQ_DMA2_0 - 7) << IRQ_DMA2_0_POS));

	bfin_write_SICB_IAR3(((CONFIG_IRQ_DMA2_1 - 7) << IRQ_DMA2_1_POS) |
			     ((CONFIG_IRQ_DMA2_2 - 7) << IRQ_DMA2_2_POS) |
			     ((CONFIG_IRQ_DMA2_3 - 7) << IRQ_DMA2_3_POS) |
			     ((CONFIG_IRQ_DMA2_4 - 7) << IRQ_DMA2_4_POS) |
			     ((CONFIG_IRQ_DMA2_5 - 7) << IRQ_DMA2_5_POS) |
			     ((CONFIG_IRQ_DMA2_6 - 7) << IRQ_DMA2_6_POS) |
			     ((CONFIG_IRQ_DMA2_7 - 7) << IRQ_DMA2_7_POS) |
			     ((CONFIG_IRQ_DMA2_8 - 7) << IRQ_DMA2_8_POS));

	bfin_write_SICB_IAR4(((CONFIG_IRQ_DMA2_9 - 7) << IRQ_DMA2_9_POS) |
			     ((CONFIG_IRQ_DMA2_10 - 7) << IRQ_DMA2_10_POS) |
			     ((CONFIG_IRQ_DMA2_11 - 7) << IRQ_DMA2_11_POS) |
			     ((CONFIG_IRQ_TIMER0 - 7) << IRQ_TIMER0_POS) |
			     ((CONFIG_IRQ_TIMER1 - 7) << IRQ_TIMER1_POS) |
			     ((CONFIG_IRQ_TIMER2 - 7) << IRQ_TIMER2_POS) |
			     ((CONFIG_IRQ_TIMER3 - 7) << IRQ_TIMER3_POS) |
			     ((CONFIG_IRQ_TIMER4 - 7) << IRQ_TIMER4_POS));

	bfin_write_SICB_IAR5(((CONFIG_IRQ_TIMER5 - 7) << IRQ_TIMER5_POS) |
			     ((CONFIG_IRQ_TIMER6 - 7) << IRQ_TIMER6_POS) |
			     ((CONFIG_IRQ_TIMER7 - 7) << IRQ_TIMER7_POS) |
			     ((CONFIG_IRQ_TIMER8 - 7) << IRQ_TIMER8_POS) |
			     ((CONFIG_IRQ_TIMER9 - 7) << IRQ_TIMER9_POS) |
			     ((CONFIG_IRQ_TIMER10 - 7) << IRQ_TIMER10_POS) |
			     ((CONFIG_IRQ_TIMER11 - 7) << IRQ_TIMER11_POS) |
			     ((CONFIG_IRQ_PROG0_INTA - 7) << IRQ_PROG0_INTA_POS));

	bfin_write_SICB_IAR6(((CONFIG_IRQ_PROG0_INTB - 7) << IRQ_PROG0_INTB_POS) |
			     ((CONFIG_IRQ_PROG1_INTA - 7) << IRQ_PROG1_INTA_POS) |
			     ((CONFIG_IRQ_PROG1_INTB - 7) << IRQ_PROG1_INTB_POS) |
			     ((CONFIG_IRQ_PROG2_INTA - 7) << IRQ_PROG2_INTA_POS) |
			     ((CONFIG_IRQ_PROG2_INTB - 7) << IRQ_PROG2_INTB_POS) |
			     ((CONFIG_IRQ_DMA1_WRRD0 - 7) << IRQ_DMA1_WRRD0_POS) |
			     ((CONFIG_IRQ_DMA1_WRRD1 - 7) << IRQ_DMA1_WRRD1_POS) |
			     ((CONFIG_IRQ_DMA2_WRRD0 - 7) << IRQ_DMA2_WRRD0_POS));

	bfin_write_SICB_IAR7(((CONFIG_IRQ_DMA2_WRRD1 - 7) << IRQ_DMA2_WRRD1_POS) |
			     ((CONFIG_IRQ_IMDMA_WRRD0 - 7) << IRQ_IMDMA_WRRD0_POS) |
			     ((CONFIG_IRQ_IMDMA_WRRD1 - 7) << IRQ_IMDMA_WRRD1_POS) |
			     ((CONFIG_IRQ_WDTIMER - 7) << IRQ_WDTIMER_POS) |
			     (0 << IRQ_RESERVED_1_POS) | (0 << IRQ_RESERVED_2_POS) |
			     (0 << IRQ_SUPPLE_0_POS) | (0 << IRQ_SUPPLE_1_POS));

	SSYNC();
}

int __init init_arch_irq(void)
{
	int irq;
	struct irq_desc *desc;
	unsigned long ilat = 0;

	bfin_write_SICB_IMASK0(SIC_UNMASK_ALL);
	bfin_write_SICB_IMASK1(SIC_UNMASK_ALL);

	irq_disable();

	for (irq = 0; irq <= SYS_IRQS; irq++) {
		desc = irq_to_desc(irq);
		if (irq <= IRQ_CORETMR)
			desc->chip = &bfin_core_irqchip;
		else
			desc->chip = &bfin_internal_irqchip;

		switch (irq) {
		/*
		case IRQ_PROG0_INTA:
		case IRQ_PROG1_INTA:
		case IRQ_PROG2_INTA:
			set_irq_chained_handler(irq,
						bfin_demux_gpio_irq);
			break;
		*/
		default:
			desc->handle_irq = handle_simple_irq;
			desc->name = NULL;
			break;
		}
	}

	/* if configured as edge, then will be changed to do_edge_IRQ */
	/*for (irq = GPIO_IRQ_BASE; irq < NR_IRQS; irq++)
		set_irq_chip_and_handler(irq, &bfin_gpio_irqchip,
					 handle_level_irq);*/


	bfin_write_IMASK(0);
	CSYNC();
	ilat = bfin_read_ILAT();
	CSYNC();
	bfin_write_ILAT(ilat);
	CSYNC();

	program_IAR();
	search_IAR();

	/* Enable interrupts IVG7-15 */
	bfin_irq_flags |= IMASK_IVG15 |
	    IMASK_IVG14 | IMASK_IVG13 | IMASK_IVG12 | IMASK_IVG11 |
	    IMASK_IVG10 | IMASK_IVG9 | IMASK_IVG8 | IMASK_IVG7 | IMASK_IVGHW;

#ifdef SICB_IWR0
	bfin_write_SICB_IWR0(IWR_DISABLE_ALL);
# ifdef SICB_IWR1
	bfin_write_SICB_IWR1(IWR_DISABLE_ALL);
# endif
# ifdef SICB_IWR2
	bfin_write_SICB_IWR2(IWR_DISABLE_ALL);
# endif
#else
	bfin_write_SICB_IWR(IWR_DISABLE_ALL);
#endif

	return 0;
}

void __init init_exception_vectors(void)
{
	bfin_write_EVT2(evt_nmi);
	bfin_write_EVT3(trap);
	bfin_write_EVT5(evt_ivhw);
	bfin_write_EVT6(evt_timer);
	bfin_write_EVT7(evt_evt7);
	bfin_write_EVT8(evt_evt8);
	bfin_write_EVT9(evt_evt9);
	bfin_write_EVT10(evt_evt10);
	bfin_write_EVT11(evt_evt11);
	bfin_write_EVT12(evt_evt12);
	bfin_write_EVT13(evt_evt13);
	bfin_write_EVT14(evt_evt14);
	bfin_write_EVT15(evt_evt15);
	CSYNC();
}

void __init bfin_core_timer_init(void)
{
	unsigned long tcount = ((get_cclk() / (HZ * TIME_SCALE)) - 1);
	bfin_write_TCNTL(TMPWR);
	CSYNC();
	bfin_write_TSCALE(TIME_SCALE - 1);
	bfin_write_TPERIOD(tcount);
	bfin_write_TCOUNT(tcount);
	CSYNC();
	bfin_write_TCNTL(TMPWR | TMREN | TAUTORLD);

	jiffies = 0;
}

static irqreturn_t ipi1_handler(int irq, void *dev)
{
	clear_ipi1();
#ifdef CONFIG_COREB_LED_INDICATOR
	ipi1_light = !ipi1_light;
	if (ipi1_light) bfin_gpio_set_value(LED5, 1);
	else bfin_gpio_set_value(LED5, 0);
#endif
	return IRQ_HANDLED;
}

static irqreturn_t timer_handler(int irq, void *dev)
{
	static unsigned long jiff = 0;
	jiffies++;
	if (time_after(jiffies, jiff)) {
		jiff = (unsigned long)jiffies + 125;
#ifdef CONFIG_COREB_LED_INDICATOR
		timer_light = !timer_light;
		if (timer_light) bfin_gpio_set_value(LED20, 1);
		else bfin_gpio_set_value(LED20, 0);
#endif
	}

	return IRQ_HANDLED;
}

static struct irqaction ipi1_irq_action = {
	.name		= "ipi",
	.flags		= IRQF_DISABLED, 
	.handler	= ipi1_handler,
	.dev_id		= NULL,
};

static struct irqaction timer_irq_action = {
	.name		= "timer",
	.flags		= IRQF_DISABLED, 
	.handler	= timer_handler,
	.dev_id		= NULL,
};

void dsp_stub()
{
	while (!asm_read(&sha->dsp_start));
}

void __init dsp_init()
{
#ifdef CONFIG_COREB_LED_INDICATOR
	coerb_led_indicator_init();
#endif
	//sha->dsp_start = 0;
	//sha->dsp_reset = 0;
	//sha->dsp_stub_addr = (unsigned long)dsp_stub;
	asm_set(&sha->dsp_start, 0);
	asm_set(&sha->dsp_reset, 0);
	asm_set(&sha->dsp_stub_addr, (unsigned long)dsp_stub);

	irq_disable();
	init_exception_vectors();
	init_arch_irq();
	trace_buffer_init();
	bfin_core_timer_init();
	irq_enable();
	setup_irq(IRQ_CORETMR, &timer_irq_action);
	setup_irq(IRQ_SUPPLE_1, &ipi1_irq_action);
}

