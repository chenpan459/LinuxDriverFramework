// SPDX-License-Identifier: GPL-2.0
/*
 * Example: loopback UART using nv_uart framework
 *
 * Creates /dev/ttyNV0 — bytes written are echoed back to the reader.
 *
 * Load:   sudo insmod nv_uart_demo.ko
 * Test:   uart/app/bin/nv_test_uart
 * Unload: sudo rmmod nv_uart_demo
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/serial_core.h>
#include <linux/tty_flip.h>

#include "nv_uart.h"

#define DRV_NAME	"nv_uart_demo"
#define TTY_NAME	"ttyNV"
#define NV_UART_PORTS	1

static struct nv_uart_driver demo_uart_drv;
static struct nv_uart_port demo_port;

static void demo_start_tx(struct nv_uart_port *np)
{
	struct uart_port *uport = &np->port;
	struct tty_port *tport;
	unsigned char ch;

	if (!uport->state)
		return;

	tport = &uport->state->port;

	while (!uart_tx_stopped(uport) && uart_fifo_get(uport, &ch))
		uart_insert_char(uport, 0, TTY_NORMAL, ch, 1);

	tty_flip_buffer_push(tport);
	nv_uart_write_wakeup(np);
}

static const struct nv_uart_port_ops demo_port_ops = {
	.start_tx	= demo_start_tx,
};

static int __init demo_init(void)
{
	int ret;

	demo_uart_drv.name = DRV_NAME;
	demo_uart_drv.owner = THIS_MODULE;
	demo_uart_drv.dev_name = TTY_NAME;
	demo_uart_drv.nr_ports = NV_UART_PORTS;

	ret = nv_uart_driver_register(&demo_uart_drv);
	if (ret)
		return ret;

	demo_port.ops = &demo_port_ops;
	demo_port.uartclk = 1843200;
	demo_port.fifosize = 32;

	ret = nv_uart_port_add(&demo_uart_drv, &demo_port, 0);
	if (ret) {
		nv_uart_driver_unregister(&demo_uart_drv);
		return ret;
	}

	pr_info("nv_uart_demo: /dev/%s0 loopback ready\n", TTY_NAME);
	return 0;
}

static void __exit demo_exit(void)
{
	nv_uart_port_remove(&demo_uart_drv, &demo_port);
	nv_uart_driver_unregister(&demo_uart_drv);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("UART demo driver (ttyNV loopback)");
MODULE_LICENSE("GPL");
