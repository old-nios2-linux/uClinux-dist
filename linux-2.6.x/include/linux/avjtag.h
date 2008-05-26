/*
 *	avjtag.h -- Altera Avalon JTAG UART driver defines.
 */

#ifndef	avjtag_h
#define	avjtag_h

#include <linux/serial_core.h>
#include <linux/platform_device.h>

struct avalon_jtaguart_platform_uart {
	unsigned long mapbase;	/* Physical address base */
	void __iomem *membase;	/* Virtual address if mapped */
	unsigned int irq;	/* Interrupt vector */
};

#endif /* avjtag_h */
