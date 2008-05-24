/*
 *	avuart.h -- Altera Avalon UART driver defines.
 *	based on : mcfuart.h
 */

#ifndef	avuart_h
#define	avuart_h

#include <linux/serial_core.h>
#include <linux/platform_device.h>

struct avalon_uart_platform_uart {
	unsigned long mapbase;	/* Physical address base */
	void __iomem *membase;	/* Virtual address if mapped */
	unsigned int irq;	/* Interrupt vector */
	unsigned int uartclk;	/* UART clock rate */
};

#endif /* avuart_h */
