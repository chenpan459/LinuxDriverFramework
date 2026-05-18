# SPDX-License-Identifier: GPL-2.0
#
#   make
#   sudo insmod nv_i2c_demo.ko

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
PWD        := $(shell pwd)

ccflags-y += -I$(src)/include

obj-m := nv_i2c_demo.o
nv_i2c_demo-y := modules/nv_i2c_demo.o core/nv_i2c.o

.PHONY: all clean

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean
