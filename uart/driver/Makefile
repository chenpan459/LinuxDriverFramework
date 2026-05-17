# SPDX-License-Identifier: GPL-2.0
#
#   make
#   sudo insmod nv_uart_demo.ko

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
PWD        := $(shell pwd)

ccflags-y += -I$(src)/include

obj-m := nv_uart_demo.o
nv_uart_demo-y := modules/nv_uart_demo.o core/nv_uart.o

.PHONY: all clean

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean
