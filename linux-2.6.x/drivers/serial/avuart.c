/*
 *	avuart.c -- Altera Avalon UART driver
 *	based on : mcf.c
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
#include <asm/avuart.h>

/*
 *	Some boards implement the DTR/DCD lines using GPIO lines, most
 *	don't. Dummy out the access macros for those that don't. Those
 *	that do should define these macros somewhere in there board
 *	specific inlude files.
 */
#if !defined(avalon_uart_getppdcd)
#define	avalon_uart_getppdcd(p)		(1)
#endif
#if !defined(avalon_uart_getppdtr)
#define	avalon_uart_getppdtr(p)		(1)
#endif
#if !defined(avalon_uart_setppdtr)
#define	avalon_uart_setppdtr(p, v)	do { } while (0)
#endif

/*
 *	Local per-uart structure.
 */
struct avalon_uart {
	struct uart_port	port;
	unsigned int		sigs;		/* Local copy of line sigs */
	unsigned short		imr;		/* Local IMR mirror */
};

static unsigned int avalon_uart_tx_empty(struct uart_port *port)
{
	return (readw(port->membase + AVALON_UART_STATUS_REG) & AVALON_UART_STATUS_TMT_MSK) ?
		TIOCSER_TEMT : 0;
}

static unsigned int avalon_uart_get_mctrl(struct uart_port *port)
{
	struct avalon_uart *pp = container_of(port, struct avalon_uart, port);
	unsigned long flags;
	unsigned int sigs;

	spin_lock_irqsave(&port->lock, flags);
	sigs = (readw(port->membase + AVALON_UART_STATUS_REG) & AVALON_UART_STATUS_CTS_MSK) ?
		0 : TIOCM_CTS;
	sigs |= (pp->sigs & TIOCM_RTS);
	sigs |= (avalon_uart_getppdcd(port->line) ? TIOCM_CD : 0);
	sigs |= (avalon_uart_getppdtr(port->line) ? TIOCM_DTR : 0);
	spin_unlock_irqrestore(&port->lock, flags);
	return sigs;
}

static void avalon_uart_set_mctrl(struct uart_port *port, unsigned int sigs)
{
	struct avalon_uart *pp = container_of(port, struct avalon_uart, port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	pp->sigs = sigs;
	avalon_uart_setppdtr(port->line, (sigs & TIOCM_DTR));
	if (sigs & TIOCM_RTS)
		pp->imr |= AVALON_UART_CONTROL_RTS_MSK;
	else
		pp->imr &= ~AVALON_UART_CONTROL_RTS_MSK;
	writew(pp->imr, port->membase + AVALON_UART_CONTROL_REG);
	spin_unlock_irqrestore(&port->lock, flags);
}

static void avalon_uart_start_tx(struct uart_port *port)
{
	struct avalon_uart *pp = container_of(port, struct avalon_uart, port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	pp->imr |= AVALON_UART_CONTROL_TRDY_MSK;
	writew(pp->imr, port->membase + AVALON_UART_CONTROL_REG);
	spin_unlock_irqrestore(&port->lock, flags);
}

static void avalon_uart_stop_tx(struct uart_port *port)
{
	struct avalon_uart *pp = container_of(port, struct avalon_uart, port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	pp->imr &= ~AVALON_UART_CONTROL_TRDY_MSK;
	writeb(pp->imr, port->membase + AVALON_UART_CONTROL_REG);
	spin_unlock_irqrestore(&port->lock, flags);
}

static void avalon_uart_stop_rx(struct uart_port *port)
{
	struct avalon_uart *pp = container_of(port, struct avalon_uart, port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	pp->imr &= ~AVALON_UART_CONTROL_RRDY_MSK;
	writew(pp->imr, port->membase + AVALON_UART_CONTROL_REG);
	spin_unlock_irqrestore(&port->lock, flags);
}

static void avalon_uart_break_ctl(struct uart_port *port, int break_state)
{
	struct avalon_uart *pp = container_of(port, struct avalon_uart, port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	if (break_state == -1)
		pp->imr |= AVALON_UART_CONTROL_TRBK_MSK;
	else
		pp->imr &= ~AVALON_UART_CONTROL_TRBK_MSK;
	writew(pp->imr, port->membase + AVALON_UART_CONTROL_REG);
	spin_unlock_irqrestore(&port->lock, flags);
}

static void avalon_uart_enable_ms(struct uart_port *port)
{
}

static int avalon_uart_startup(struct uart_port *port)
{
	struct avalon_uart *pp = container_of(port, struct avalon_uart, port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	/* Enable RX interrupts now */
	pp->imr = AVALON_UART_CONTROL_RRDY_MSK;
	writew(pp->imr, port->membase + AVALON_UART_CONTROL_REG);

	spin_unlock_irqrestore(&port->lock, flags);

	return 0;
}

static void avalon_uart_shutdown(struct uart_port *port)
{
	struct avalon_uart *pp = container_of(port, struct avalon_uart, port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	/* Disable all interrupts now */
	pp->imr = 0;
	writew(pp->imr, port->membase + AVALON_UART_CONTROL_REG);

	spin_unlock_irqrestore(&port->lock, flags);
}

static void avalon_uart_set_termios(struct uart_port *port, struct ktermios *termios,
	struct ktermios *old)
{
	unsigned long flags;
	unsigned int baud, baudclk;

	baud = uart_get_baud_rate(port, termios, old, 0, 4000000);
	baudclk = port->uartclk / baud;

	spin_lock_irqsave(&port->lock, flags);
	writew(baudclk, port->membase + AVALON_UART_DIVISOR_REG);
	spin_unlock_irqrestore(&port->lock, flags);
}

static void avalon_uart_rx_chars(struct avalon_uart *pp)
{
	struct uart_port *port = &pp->port;
	unsigned char ch, flag;
	unsigned short status;

	while ((status = readw(port->membase + AVALON_UART_STATUS_REG)) &
			AVALON_UART_STATUS_RRDY_MSK) {
		ch = readb(port->membase + AVALON_UART_RXDATA_REG);
		flag = TTY_NORMAL;
		port->icount.rx++;

		if (status & AVALON_UART_STATUS_E_MSK) {
			writew(status,
				port->membase + AVALON_UART_STATUS_REG);

			if (status & AVALON_UART_STATUS_BRK_MSK) {
				port->icount.brk++;
				if (uart_handle_break(port))
					continue;
			} else if (status & AVALON_UART_STATUS_PE_MSK) {
				port->icount.parity++;
			} else if (status & AVALON_UART_STATUS_ROE_MSK) {
				port->icount.overrun++;
			} else if (status & AVALON_UART_STATUS_FE_MSK) {
				port->icount.frame++;
			}

			status &= port->read_status_mask;

			if (status & AVALON_UART_STATUS_BRK_MSK)
				flag = TTY_BREAK;
			else if (status & AVALON_UART_STATUS_PE_MSK)
				flag = TTY_PARITY;
			else if (status & AVALON_UART_STATUS_FE_MSK)
				flag = TTY_FRAME;
		}

		if (uart_handle_sysrq_char(port, ch))
			continue;
		uart_insert_char(port, status, AVALON_UART_STATUS_ROE_MSK, ch, flag);
	}

	tty_flip_buffer_push(port->info->tty);
}

static void avalon_uart_tx_chars(struct avalon_uart *pp)
{
	struct uart_port *port = &pp->port;
	struct circ_buf *xmit = &port->info->xmit;

	if (port->x_char) {
		/* Send special char - probably flow control */
		writeb(port->x_char, port->membase + AVALON_UART_TXDATA_REG);
		port->x_char = 0;
		port->icount.tx++;
		return;
	}

	while (readw(port->membase + AVALON_UART_STATUS_REG) & AVALON_UART_STATUS_TRDY_MSK) {
		if (xmit->head == xmit->tail)
			break;
		writeb(xmit->buf[xmit->tail], port->membase + AVALON_UART_TXDATA_REG);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE -1);
		port->icount.tx++;
	}

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	if (xmit->head == xmit->tail) {
		pp->imr &= ~AVALON_UART_CONTROL_TRDY_MSK;
		writeb(pp->imr, port->membase + AVALON_UART_CONTROL_REG);
	}
}

static irqreturn_t avalon_uart_interrupt(int irq, void *data)
{
	struct uart_port *port = data;
	struct avalon_uart *pp = container_of(port, struct avalon_uart, port);
	unsigned int isr;

	isr = readb(port->membase + AVALON_UART_STATUS_REG) & pp->imr;
	if (isr & AVALON_UART_STATUS_RRDY_MSK)
		avalon_uart_rx_chars(pp);
	if (isr & AVALON_UART_STATUS_TRDY_MSK)
		avalon_uart_tx_chars(pp);
	return IRQ_HANDLED;
}

static void avalon_uart_config_port(struct uart_port *port, int flags)
{
	port->type = PORT_AVALON_UART;

	/* Clear mask, so no surprise interrupts. */
	writeb(0, port->membase + AVALON_UART_CONTROL_REG);

	if (request_irq(port->irq, avalon_uart_interrupt, IRQF_DISABLED, "UART", port))
		printk(KERN_ERR "AVALON_UART: unable to attach Avalon UART %d "
			"interrupt vector=%d\n", port->line, port->irq);
}

static const char *avalon_uart_type(struct uart_port *port)
{
	return (port->type == PORT_AVALON_UART) ? "Avalon UART" : NULL;
}

static int avalon_uart_request_port(struct uart_port *port)
{
	/* UARTs always present */
	return 0;
}

static void avalon_uart_release_port(struct uart_port *port)
{
	/* Nothing to release... */
}

static int avalon_uart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	if ((ser->type != PORT_UNKNOWN) && (ser->type != PORT_AVALON_UART))
		return -EINVAL;
	return 0;
}

/*
 *	Define the basic serial functions we support.
 */
static struct uart_ops avalon_uart_ops = {
	.tx_empty	= avalon_uart_tx_empty,
	.get_mctrl	= avalon_uart_get_mctrl,
	.set_mctrl	= avalon_uart_set_mctrl,
	.start_tx	= avalon_uart_start_tx,
	.stop_tx	= avalon_uart_stop_tx,
	.stop_rx	= avalon_uart_stop_rx,
	.enable_ms	= avalon_uart_enable_ms,
	.break_ctl	= avalon_uart_break_ctl,
	.startup	= avalon_uart_startup,
	.shutdown	= avalon_uart_shutdown,
	.set_termios	= avalon_uart_set_termios,
	.type		= avalon_uart_type,
	.request_port	= avalon_uart_request_port,
	.release_port	= avalon_uart_release_port,
	.config_port	= avalon_uart_config_port,
	.verify_port	= avalon_uart_verify_port,
};

static struct avalon_uart avalon_uart_ports[CONFIG_SERIAL_AVALON_UART_MAXPORTS];

#if defined(CONFIG_SERIAL_AVALON_UART_CONSOLE)

int __init early_avalon_uart_setup(struct avalon_uart_platform_uart *platp)
{
	struct uart_port *port;
	int i;

	for (i = 0; ((i < CONFIG_SERIAL_AVALON_UART_MAXPORTS) && (platp[i].mapbase)); i++) {
		port = &avalon_uart_ports[i].port;

		port->line = i;
		port->type = PORT_AVALON_UART;
		port->mapbase = platp[i].mapbase;
		port->membase = (platp[i].membase) ? platp[i].membase :
			(unsigned char __iomem *) port->mapbase;
		port->iotype = SERIAL_IO_MEM;
		port->irq = platp[i].irq;
		port->uartclk = platp[i].uartclk;
		port->flags = ASYNC_BOOT_AUTOCONF;
		port->ops = &avalon_uart_ops;
	}

	return 0;
}

static void avalon_uart_console_putc(struct console *co, const char c)
{
	struct uart_port *port = &(avalon_uart_ports + co->index)->port;
	int i;

	for (i = 0; (i < 0x10000); i++) {
		if (readw(port->membase + AVALON_UART_STATUS_REG) & AVALON_UART_STATUS_TRDY_MSK)
			break;
	}
	writeb(c, port->membase + AVALON_UART_TXDATA_REG);
	for (i = 0; (i < 0x10000); i++) {
		if (readw(port->membase + AVALON_UART_STATUS_REG) & AVALON_UART_STATUS_TRDY_MSK)
			break;
	}
}

static void avalon_uart_console_write(struct console *co, const char *s, unsigned int count)
{
	for (; (count); count--, s++) {
		avalon_uart_console_putc(co, *s);
		if (*s == '\n')
			avalon_uart_console_putc(co, '\r');
	}
}

static int __init avalon_uart_console_setup(struct console *co, char *options)
{
	struct uart_port *port;
	int baud = CONFIG_SERIAL_AVALON_UART_BAUDRATE;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	if ((co->index >= 0) && (co->index <= CONFIG_SERIAL_AVALON_UART_MAXPORTS))
		co->index = 0;
	port = &avalon_uart_ports[co->index].port;
	if (port->membase == 0)
		return -ENODEV;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct uart_driver avalon_uart_driver;

static struct console avalon_uart_console = {
	.name		= "ttyS",
	.write		= avalon_uart_console_write,
	.device		= uart_console_device,
	.setup		= avalon_uart_console_setup,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
	.data		= &avalon_uart_driver,
};

static int __init avalon_uart_console_init(void)
{
	register_console(&avalon_uart_console);
	return 0;
}

console_initcall(avalon_uart_console_init);

#define	AVALON_UART_CONSOLE	&avalon_uart_console

#else

#define	AVALON_UART_CONSOLE	NULL

#endif /* CONFIG_AVALON_UART_CONSOLE */

/*
 *	Define the avalon_uart UART driver structure.
 */
static struct uart_driver avalon_uart_driver = {
	.owner		= THIS_MODULE,
	.driver_name	= "avalon_uart",
	.dev_name	= "ttyS",
	.major		= TTY_MAJOR,
	.minor		= 64,
	.nr		= CONFIG_SERIAL_AVALON_UART_MAXPORTS,
	.cons		= AVALON_UART_CONSOLE,
};

static int __devinit avalon_uart_probe(struct platform_device *pdev)
{
	struct avalon_uart_platform_uart *platp = pdev->dev.platform_data;
	struct uart_port *port;
	int i;

	for (i = 0; ((i < CONFIG_SERIAL_AVALON_UART_MAXPORTS) && (platp[i].mapbase)); i++) {
		port = &avalon_uart_ports[i].port;

		port->line = i;
		port->type = PORT_AVALON_UART;
		port->mapbase = platp[i].mapbase;
		port->membase = (platp[i].membase) ? platp[i].membase :
			(unsigned char __iomem *) platp[i].mapbase;
		port->iotype = SERIAL_IO_MEM;
		port->irq = platp[i].irq;
		port->uartclk = platp[i].uartclk;
		port->ops = &avalon_uart_ops;
		port->flags = ASYNC_BOOT_AUTOCONF;

		uart_add_one_port(&avalon_uart_driver, port);
	}

	return 0;
}

static int avalon_uart_remove(struct platform_device *pdev)
{
	struct uart_port *port;
	int i;

	for (i = 0; (i < CONFIG_SERIAL_AVALON_UART_MAXPORTS); i++) {
		port = &avalon_uart_ports[i].port;
		if (port)
			uart_remove_one_port(&avalon_uart_driver, port);
	}

	return 0;
}

static struct platform_driver avalon_uart_platform_driver = {
	.probe		= avalon_uart_probe,
	.remove		= __devexit_p(avalon_uart_remove),
	.driver		= {
		.name	= "avalon_uart",
		.owner	= THIS_MODULE,
	},
};

static int __init avalon_uart_init(void)
{
	int rc;

	rc = uart_register_driver(&avalon_uart_driver);
	if (rc)
		return rc;
	rc = platform_driver_register(&avalon_uart_platform_driver);
	if (rc)
		return rc;
	return 0;
}

static void __exit avalon_uart_exit(void)
{
	platform_driver_unregister(&avalon_uart_platform_driver);
	uart_unregister_driver(&avalon_uart_driver);
}

module_init(avalon_uart_init);
module_exit(avalon_uart_exit);

MODULE_LICENSE("GPL");
