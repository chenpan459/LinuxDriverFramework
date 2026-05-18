// SPDX-License-Identifier: GPL-2.0
/*
 * LinuxDriverFramework - UART helper implementation (serial_core)
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/serial_core.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>

#include "nv_uart.h"

struct nv_uart_port *nv_uart_port_from_port(struct uart_port *port)
{
	return port ? container_of(port, struct nv_uart_port, port) : NULL;
}
EXPORT_SYMBOL_GPL(nv_uart_port_from_port);

int nv_uart_push_rx(struct nv_uart_port *np, const unsigned char *buf,
		    int count)
{
	struct uart_port *port;
	struct tty_port *tport;
	unsigned long flags;
	int i, pushed = 0;

	if (!np || !buf || count <= 0)
		return -EINVAL;

	port = &np->port;
	if (!port->state)
		return -ENODEV;

	tport = &port->state->port;

	spin_lock_irqsave(&port->lock, flags);
	for (i = 0; i < count; i++) {
		uart_insert_char(port, 0, TTY_NORMAL, buf[i], 1);
		pushed++;
	}
	spin_unlock_irqrestore(&port->lock, flags);

	tty_flip_buffer_push(tport);
	return pushed;
}
EXPORT_SYMBOL_GPL(nv_uart_push_rx);

void nv_uart_write_wakeup(struct nv_uart_port *np)
{
	if (np)
		uart_write_wakeup(&np->port);
}
EXPORT_SYMBOL_GPL(nv_uart_write_wakeup);

static struct nv_uart_port *nv_uart_np(struct uart_port *port)
{
	return nv_uart_port_from_port(port);
}

static unsigned int nv_uart_tx_empty(struct uart_port *port)
{
	struct nv_uart_port *np = nv_uart_np(port);

	if (np && np->ops && np->ops->tx_empty)
		return np->ops->tx_empty(np);

	return TIOCSER_TEMT;
}

static void nv_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	struct nv_uart_port *np = nv_uart_np(port);

	if (np && np->ops && np->ops->set_mctrl)
		np->ops->set_mctrl(np, mctrl);
	else
		port->mctrl = mctrl;
}

static unsigned int nv_uart_get_mctrl(struct uart_port *port)
{
	struct nv_uart_port *np = nv_uart_np(port);

	if (np && np->ops && np->ops->get_mctrl)
		return np->ops->get_mctrl(np);

	return TIOCM_CTS | TIOCM_DSR | TIOCM_CAR;
}

static void nv_uart_stop_tx(struct uart_port *port)
{
	struct nv_uart_port *np = nv_uart_np(port);

	if (np && np->ops && np->ops->stop_tx)
		np->ops->stop_tx(np);
}

static void nv_uart_start_tx(struct uart_port *port)
{
	struct nv_uart_port *np = nv_uart_np(port);

	if (np && np->ops && np->ops->start_tx)
		np->ops->start_tx(np);
}

static void nv_uart_stop_rx(struct uart_port *port)
{
}

static void nv_uart_start_rx(struct uart_port *port)
{
}

static void nv_uart_enable_ms(struct uart_port *port)
{
}

static int nv_uart_startup(struct uart_port *port)
{
	struct nv_uart_port *np = nv_uart_np(port);

	if (np && np->ops && np->ops->startup)
		return np->ops->startup(np);

	return 0;
}

static void nv_uart_shutdown(struct uart_port *port)
{
	struct nv_uart_port *np = nv_uart_np(port);

	if (np && np->ops && np->ops->shutdown)
		np->ops->shutdown(np);
}

static void nv_uart_set_termios(struct uart_port *port, struct ktermios *new,
				const struct ktermios *old)
{
	struct nv_uart_port *np = nv_uart_np(port);
	unsigned int baud;

	baud = uart_get_baud_rate(port, new, old, 0, port->uartclk / 16);
	uart_update_timeout(port, new->c_cflag, baud);

	if (np && np->ops && np->ops->set_termios)
		np->ops->set_termios(np, new, old);
}

static const char *nv_uart_type(struct uart_port *port)
{
	return "nv_uart";
}

static void nv_uart_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE)
		port->type = PORT_UNKNOWN;
}

static int nv_uart_request_port(struct uart_port *port)
{
	return 0;
}

static void nv_uart_release_port(struct uart_port *port)
{
}

static const struct uart_ops nv_uart_core_ops = {
	.tx_empty	= nv_uart_tx_empty,
	.set_mctrl	= nv_uart_set_mctrl,
	.get_mctrl	= nv_uart_get_mctrl,
	.stop_tx	= nv_uart_stop_tx,
	.start_tx	= nv_uart_start_tx,
	.stop_rx	= nv_uart_stop_rx,
	.start_rx	= nv_uart_start_rx,
	.enable_ms	= nv_uart_enable_ms,
	.startup	= nv_uart_startup,
	.shutdown	= nv_uart_shutdown,
	.set_termios	= nv_uart_set_termios,
	.type		= nv_uart_type,
	.release_port	= nv_uart_release_port,
	.request_port	= nv_uart_request_port,
	.config_port	= nv_uart_config_port,
};

int nv_uart_port_add(struct nv_uart_driver *drv, struct nv_uart_port *np,
		     int line)
{
	struct uart_port *port;
	int ret;

	if (!drv || !np || !drv->registered || line < 0 || line >= drv->nr_ports)
		return -EINVAL;

	if (np->added)
		return -EBUSY;

	port = &np->port;
	memset(port, 0, sizeof(*port));
	spin_lock_init(&port->lock);

	port->ops = &nv_uart_core_ops;
	port->type = PORT_UNKNOWN;
	port->iotype = UPIO_MEM;
	port->line = line;
	port->uartclk = np->uartclk ? np->uartclk : 1843200;
	port->fifosize = np->fifosize ? np->fifosize : 16;
	port->flags = UPF_BOOT_AUTOCONF | UPF_LOW_LATENCY;

	np->driver = drv;
	np->line = line;

	ret = uart_add_one_port(&drv->uart_drv, port);
	if (ret)
		return ret;

	np->added = true;
	return 0;
}
EXPORT_SYMBOL_GPL(nv_uart_port_add);

void nv_uart_port_remove(struct nv_uart_driver *drv, struct nv_uart_port *np)
{
	if (!drv || !np || !np->added)
		return;

	uart_remove_one_port(&drv->uart_drv, &np->port);
	np->added = false;
}
EXPORT_SYMBOL_GPL(nv_uart_port_remove);

int nv_uart_driver_register(struct nv_uart_driver *drv)
{
	int ret;

	if (!drv || !drv->name || !drv->owner || !drv->dev_name || drv->nr_ports < 1)
		return -EINVAL;

	if (drv->registered)
		return -EBUSY;

	drv->uart_drv.owner = drv->owner;
	drv->uart_drv.driver_name = drv->name;
	drv->uart_drv.dev_name = drv->dev_name;
	drv->uart_drv.nr = drv->nr_ports;
	drv->uart_drv.major = 0;
	drv->uart_drv.minor = 0;

	ret = uart_register_driver(&drv->uart_drv);
	if (ret)
		return ret;

	drv->registered = true;
	return 0;
}
EXPORT_SYMBOL_GPL(nv_uart_driver_register);

void nv_uart_driver_unregister(struct nv_uart_driver *drv)
{
	if (!drv || !drv->registered)
		return;

	uart_unregister_driver(&drv->uart_drv);
	drv->registered = false;
}
EXPORT_SYMBOL_GPL(nv_uart_driver_unregister);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("UART serial framework");
MODULE_LICENSE("GPL");
