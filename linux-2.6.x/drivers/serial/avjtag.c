/*
 *	avjtag.c -- Altera Avalon JTAG UART driver
 *
 *	Based on mcf.c -- Freescale ColdFire UART driver
 *
 *	(C) Copyright 2003-2007, Greg Ungerer <gerg@snapgear.com>
 *	(C) Copyright 2008, Thomas Chou <thomas@wytron.com.tw>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/avjtag.h>

/*
 *	Avalon JATG UART reg defs
 */

#define AVALON_JTAGUART_DATA_REG                  0

#define AVALON_JTAGUART_DATA_DATA_MSK             (0x000000FF)
#define AVALON_JTAGUART_DATA_RVALID_MSK           (0x00008000)
#define AVALON_JTAGUART_DATA_RAVAIL_MSK           (0xFFFF0000)
#define AVALON_JTAGUART_DATA_RAVAIL_OFST          (16)

#define AVALON_JTAGUART_CONTROL_REG               4

#define AVALON_JTAGUART_CONTROL_RE_MSK            (0x00000001)
#define AVALON_JTAGUART_CONTROL_WE_MSK            (0x00000002)
#define AVALON_JTAGUART_CONTROL_RI_MSK            (0x00000100)
#define AVALON_JTAGUART_CONTROL_RI_OFST           (8)
#define AVALON_JTAGUART_CONTROL_WI_MSK            (0x00000200)
#define AVALON_JTAGUART_CONTROL_AC_MSK            (0x00000400)
#define AVALON_JTAGUART_CONTROL_WSPACE_MSK        (0xFFFF0000)
#define AVALON_JTAGUART_CONTROL_WSPACE_OFST       (16)

/*
 *	Local per-uart structure.
 */
struct avalon_jtaguart {
	struct uart_port port;
	unsigned int sigs;	/* Local copy of line sigs */
	unsigned long imr;	/* Local IMR mirror */
};

static unsigned int avalon_jtaguart_tx_empty(struct uart_port *port)
{
	return (readl(port->membase + AVALON_JTAGUART_CONTROL_REG) &
		AVALON_JTAGUART_CONTROL_WSPACE_MSK) ? TIOCSER_TEMT : 0;
}

static unsigned int avalon_jtaguart_get_mctrl(struct uart_port *port)
{
	return TIOCM_CAR | TIOCM_DSR | TIOCM_CTS;
}

static void avalon_jtaguart_set_mctrl(struct uart_port *port, unsigned int sigs)
{
}

static void avalon_jtaguart_start_tx(struct uart_port *port)
{
	struct avalon_jtaguart *pp =
	    container_of(port, struct avalon_jtaguart, port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	pp->imr |= AVALON_JTAGUART_CONTROL_WE_MSK;
	writel(pp->imr, port->membase + AVALON_JTAGUART_CONTROL_REG);
	spin_unlock_irqrestore(&port->lock, flags);
}

static void avalon_jtaguart_stop_tx(struct uart_port *port)
{
	struct avalon_jtaguart *pp =
	    container_of(port, struct avalon_jtaguart, port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	pp->imr &= ~AVALON_JTAGUART_CONTROL_WE_MSK;
	writel(pp->imr, port->membase + AVALON_JTAGUART_CONTROL_REG);
	spin_unlock_irqrestore(&port->lock, flags);
}

static void avalon_jtaguart_stop_rx(struct uart_port *port)
{
	struct avalon_jtaguart *pp =
	    container_of(port, struct avalon_jtaguart, port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	pp->imr &= ~AVALON_JTAGUART_CONTROL_RE_MSK;
	writel(pp->imr, port->membase + AVALON_JTAGUART_CONTROL_REG);
	spin_unlock_irqrestore(&port->lock, flags);
}

static void avalon_jtaguart_break_ctl(struct uart_port *port, int break_state)
{
}

static void avalon_jtaguart_enable_ms(struct uart_port *port)
{
}

static int avalon_jtaguart_startup(struct uart_port *port)
{
	struct avalon_jtaguart *pp =
	    container_of(port, struct avalon_jtaguart, port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	/* Enable RX interrupts now */
	pp->imr = AVALON_JTAGUART_CONTROL_RE_MSK;
	writew(pp->imr, port->membase + AVALON_JTAGUART_CONTROL_REG);

	spin_unlock_irqrestore(&port->lock, flags);

	return 0;
}

static void avalon_jtaguart_shutdown(struct uart_port *port)
{
	struct avalon_jtaguart *pp =
	    container_of(port, struct avalon_jtaguart, port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	/* Disable all interrupts now */
	pp->imr = 0;
	writel(pp->imr, port->membase + AVALON_JTAGUART_CONTROL_REG);

	spin_unlock_irqrestore(&port->lock, flags);
}

static void avalon_jtaguart_set_termios(struct uart_port *port,
					struct ktermios *termios,
					struct ktermios *old)
{
}

static void avalon_jtaguart_rx_chars(struct avalon_jtaguart *pp)
{
	struct uart_port *port = &pp->port;
	unsigned char ch, flag;
	unsigned long status;

	while ((status = readl(port->membase + AVALON_JTAGUART_DATA_REG)) &
	       AVALON_JTAGUART_DATA_RVALID_MSK) {
		ch = status & AVALON_JTAGUART_DATA_DATA_MSK;
		flag = TTY_NORMAL;
		port->icount.rx++;

		if (uart_handle_sysrq_char(port, ch))
			continue;
		uart_insert_char(port, 0, 0, ch, flag);
	}

	tty_flip_buffer_push(port->info->tty);
}

static void avalon_jtaguart_tx_chars(struct avalon_jtaguart *pp)
{
	struct uart_port *port = &pp->port;
	struct circ_buf *xmit = &port->info->xmit;

	if (port->x_char) {
		/* Send special char - probably flow control */
		writeb(port->x_char, port->membase + AVALON_JTAGUART_DATA_REG);
		port->x_char = 0;
		port->icount.tx++;
		return;
	}

	while (readl(port->membase + AVALON_JTAGUART_CONTROL_REG) &
	       AVALON_JTAGUART_CONTROL_WSPACE_MSK) {
		if (xmit->head == xmit->tail)
			break;
		writeb(xmit->buf[xmit->tail],
		       port->membase + AVALON_JTAGUART_DATA_REG);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
	}

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	if (xmit->head == xmit->tail) {
		pp->imr &= ~AVALON_JTAGUART_CONTROL_WE_MSK;
		writeb(pp->imr, port->membase + AVALON_JTAGUART_CONTROL_REG);
	}
}

static irqreturn_t avalon_jtaguart_interrupt(int irq, void *data)
{
	struct uart_port *port = data;
	struct avalon_jtaguart *pp =
	    container_of(port, struct avalon_jtaguart, port);
	unsigned int isr;

	isr =
	    (readl(port->membase + AVALON_JTAGUART_CONTROL_REG) >>
	     AVALON_JTAGUART_CONTROL_RI_OFST) & pp->imr;
	if (isr & AVALON_JTAGUART_CONTROL_RE_MSK)
		avalon_jtaguart_rx_chars(pp);
	if (isr & AVALON_JTAGUART_CONTROL_WE_MSK)
		avalon_jtaguart_tx_chars(pp);
	return IRQ_RETVAL(isr);
}

static void avalon_jtaguart_config_port(struct uart_port *port, int flags)
{
	port->type = PORT_AVALON_JTAGUART;

	/* Clear mask, so no surprise interrupts. */
	writeb(0, port->membase + AVALON_JTAGUART_CONTROL_REG);

	if (request_irq
	    (port->irq, avalon_jtaguart_interrupt, IRQF_DISABLED | IRQF_SHARED,
	     "JTAGUART", port))
		printk(KERN_ERR
		       "AVALON_JTAGUART: unable to attach Avalon JTAG UART %d "
		       "interrupt vector=%d\n", port->line, port->irq);
}

static const char *avalon_jtaguart_type(struct uart_port *port)
{
	return (port->type == PORT_AVALON_JTAGUART) ? "Avalon JTAG UART" : NULL;
}

static int avalon_jtaguart_request_port(struct uart_port *port)
{
	/* UARTs always present */
	return 0;
}

static void avalon_jtaguart_release_port(struct uart_port *port)
{
	/* Nothing to release... */
}

static int avalon_jtaguart_verify_port(struct uart_port *port,
				       struct serial_struct *ser)
{
	if ((ser->type != PORT_UNKNOWN) && (ser->type != PORT_AVALON_JTAGUART))
		return -EINVAL;
	return 0;
}

/*
 *	Define the basic serial functions we support.
 */
static struct uart_ops avalon_jtaguart_ops = {
	.tx_empty = avalon_jtaguart_tx_empty,
	.get_mctrl = avalon_jtaguart_get_mctrl,
	.set_mctrl = avalon_jtaguart_set_mctrl,
	.start_tx = avalon_jtaguart_start_tx,
	.stop_tx = avalon_jtaguart_stop_tx,
	.stop_rx = avalon_jtaguart_stop_rx,
	.enable_ms = avalon_jtaguart_enable_ms,
	.break_ctl = avalon_jtaguart_break_ctl,
	.startup = avalon_jtaguart_startup,
	.shutdown = avalon_jtaguart_shutdown,
	.set_termios = avalon_jtaguart_set_termios,
	.type = avalon_jtaguart_type,
	.request_port = avalon_jtaguart_request_port,
	.release_port = avalon_jtaguart_release_port,
	.config_port = avalon_jtaguart_config_port,
	.verify_port = avalon_jtaguart_verify_port,
};

#define AVALON_JTAGUART_MAXPORTS 1
static struct avalon_jtaguart avalon_jtaguart_ports[AVALON_JTAGUART_MAXPORTS];

#if defined(CONFIG_SERIAL_AVALON_JTAGUART_CONSOLE)

int __init early_avalon_jtaguart_setup(struct avalon_jtaguart_platform_uart
				       *platp)
{
	struct uart_port *port;
	int i;

	for (i = 0; ((i < AVALON_JTAGUART_MAXPORTS) && (platp[i].mapbase)); i++) {
		port = &avalon_jtaguart_ports[i].port;

		port->line = i;
		port->type = PORT_AVALON_JTAGUART;
		port->mapbase = platp[i].mapbase;
		port->membase = (platp[i].membase) ? platp[i].membase :
		    (unsigned char __iomem *)port->mapbase;
		port->iotype = SERIAL_IO_MEM;
		port->irq = platp[i].irq;
		port->flags = ASYNC_BOOT_AUTOCONF;
		port->ops = &avalon_jtaguart_ops;
	}

	return 0;
}

#ifdef CONFIG_SERIAL_AVALON_JTAGUART_CONSOLE_BYPASS
static void avalon_jtaguart_console_putc(struct console *co, const char c)
{
	struct uart_port *port = &(avalon_jtaguart_ports + co->index)->port;
	unsigned long status;

	while ((status = readl(port->membase + AVALON_JTAGUART_CONTROL_REG) &
		AVALON_JTAGUART_CONTROL_WSPACE_MSK) == 0) {
		if ((status & AVALON_JTAGUART_CONTROL_AC_MSK) == 0)
			return;	/* no connection activity */
	}
	writeb(c, port->membase + AVALON_JTAGUART_DATA_REG);
}
#else
static void avalon_jtaguart_console_putc(struct console *co, const char c)
{
	struct uart_port *port = &(avalon_jtaguart_ports + co->index)->port;
	unsigned long status;

	while ((status = readl(port->membase + AVALON_JTAGUART_CONTROL_REG) &
		AVALON_JTAGUART_CONTROL_WSPACE_MSK) == 0) ;
	writeb(c, port->membase + AVALON_JTAGUART_DATA_REG);
}
#endif

static void avalon_jtaguart_console_write(struct console *co, const char *s,
					  unsigned int count)
{
	for (; (count); count--, s++) {
		avalon_jtaguart_console_putc(co, *s);
		if (*s == '\n')
			avalon_jtaguart_console_putc(co, '\r');
	}
}

static int __init avalon_jtaguart_console_setup(struct console *co,
						char *options)
{
	struct uart_port *port;

	if ((co->index >= 0)
	    && (co->index <= AVALON_JTAGUART_MAXPORTS))
		co->index = 0;
	port = &avalon_jtaguart_ports[co->index].port;
	if (port->membase == 0)
		return -ENODEV;
	return 0;
}

static struct uart_driver avalon_jtaguart_driver;

static struct console avalon_jtaguart_console = {
	.name = "ttyJ",
	.write = avalon_jtaguart_console_write,
	.device = uart_console_device,
	.setup = avalon_jtaguart_console_setup,
	.flags = CON_PRINTBUFFER,
	.index = -1,
	.data = &avalon_jtaguart_driver,
};

static int __init avalon_jtaguart_console_init(void)
{
	register_console(&avalon_jtaguart_console);
	return 0;
}

console_initcall(avalon_jtaguart_console_init);

#define	AVALON_JTAGUART_CONSOLE	&avalon_jtaguart_console

#else

#define	AVALON_JTAGUART_CONSOLE	NULL

#endif /* CONFIG_AVALON_JTAGUART_CONSOLE */

/*
 *	Define the avalon_jtaguart UART driver structure.
 */
static struct uart_driver avalon_jtaguart_driver = {
	.owner = THIS_MODULE,
	.driver_name = "avalon_jtaguart",
	.dev_name = "ttyJ",
	.major = 232,
	.minor = 16,
	.nr = AVALON_JTAGUART_MAXPORTS,
	.cons = AVALON_JTAGUART_CONSOLE,
};

static int __devinit avalon_jtaguart_probe(struct platform_device *pdev)
{
	struct avalon_jtaguart_platform_uart *platp = pdev->dev.platform_data;
	struct uart_port *port;
	int i;

	for (i = 0; ((i < AVALON_JTAGUART_MAXPORTS) && (platp[i].mapbase)); i++) {
		port = &avalon_jtaguart_ports[i].port;

		port->line = i;
		port->type = PORT_AVALON_JTAGUART;
		port->mapbase = platp[i].mapbase;
		port->membase = (platp[i].membase) ? platp[i].membase :
		    (unsigned char __iomem *)platp[i].mapbase;
		port->iotype = SERIAL_IO_MEM;
		port->irq = platp[i].irq;
		port->ops = &avalon_jtaguart_ops;
		port->flags = ASYNC_BOOT_AUTOCONF;

		uart_add_one_port(&avalon_jtaguart_driver, port);
	}

	return 0;
}

static int avalon_jtaguart_remove(struct platform_device *pdev)
{
	struct uart_port *port;
	int i;

	for (i = 0; (i < AVALON_JTAGUART_MAXPORTS); i++) {
		port = &avalon_jtaguart_ports[i].port;
		if (port)
			uart_remove_one_port(&avalon_jtaguart_driver, port);
	}

	return 0;
}

static struct platform_driver avalon_jtaguart_platform_driver = {
	.probe = avalon_jtaguart_probe,
	.remove = __devexit_p(avalon_jtaguart_remove),
	.driver = {
		   .name = "avalon_jtaguart",
		   .owner = THIS_MODULE,
		   },
};

static int __init avalon_jtaguart_init(void)
{
	int rc;

	rc = uart_register_driver(&avalon_jtaguart_driver);
	if (rc)
		return rc;
	rc = platform_driver_register(&avalon_jtaguart_platform_driver);
	if (rc)
		return rc;
	return 0;
}

static void __exit avalon_jtaguart_exit(void)
{
	platform_driver_unregister(&avalon_jtaguart_platform_driver);
	uart_unregister_driver(&avalon_jtaguart_driver);
}

module_init(avalon_jtaguart_init);
module_exit(avalon_jtaguart_exit);

MODULE_LICENSE("GPL");
