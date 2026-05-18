/* SPDX-License-Identifier: GPL-2.0 */
/*
 * LinuxDriverFramework - UART (serial_core) helper layer
 */
#ifndef _NV_UART_H_
#define _NV_UART_H_

#include <linux/module.h>
#include <linux/serial_core.h>
#include <linux/types.h>

struct nv_uart_port;
struct nv_uart_driver;

/**
 * struct nv_uart_port_ops - low-level UART callbacks (optional entries)
 */
struct nv_uart_port_ops {
	int (*startup)(struct nv_uart_port *port);
	void (*shutdown)(struct nv_uart_port *port);
	void (*set_termios)(struct nv_uart_port *port, struct ktermios *new,
			    const struct ktermios *old);
	unsigned int (*tx_empty)(struct nv_uart_port *port);
	void (*start_tx)(struct nv_uart_port *port);
	void (*stop_tx)(struct nv_uart_port *port);
	unsigned int (*get_mctrl)(struct nv_uart_port *port);
	void (*set_mctrl)(struct nv_uart_port *port, unsigned int mctrl);
};

/**
 * struct nv_uart_port - one serial port (embed struct uart_port)
 */
struct nv_uart_port {
	struct uart_port port;
	struct nv_uart_driver *driver;
	const struct nv_uart_port_ops *ops;
	void *priv;

	unsigned int uartclk;
	unsigned int fifosize;
	int line;
	bool added;
};

/**
 * struct nv_uart_driver - uart_driver registration wrapper
 */
struct nv_uart_driver {
	const char *name;
	struct module *owner;
	const char *dev_name;
	int nr_ports;
	unsigned int flags;

	struct uart_driver uart_drv;
	bool registered;
};

#define NV_UART_FLAG_CONSOLE		BIT(0)

int nv_uart_driver_register(struct nv_uart_driver *drv);
void nv_uart_driver_unregister(struct nv_uart_driver *drv);

int nv_uart_port_add(struct nv_uart_driver *drv, struct nv_uart_port *port,
		     int line);
void nv_uart_port_remove(struct nv_uart_driver *drv, struct nv_uart_port *port);

struct nv_uart_port *nv_uart_port_from_port(struct uart_port *port);

int nv_uart_push_rx(struct nv_uart_port *port, const unsigned char *buf,
		    int count);
void nv_uart_write_wakeup(struct nv_uart_port *port);

static inline struct device *nv_uart_port_device(struct nv_uart_port *port)
{
	return port && port->port.dev ? port->port.dev : NULL;
}

#endif /* _NV_UART_H_ */
