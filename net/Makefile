# SPDX-License-Identifier: GPL-2.0
# Out-of-tree build for net framework + example
#
#   make
#   sudo insmod nv_net_dummy.ko
#   ip link show
#   sudo rmmod nv_net_dummy

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
PWD        := $(shell pwd)

ccflags-y += -I$(src)/include

obj-m := nv_net_dummy.o
nv_net_dummy-y := examples/nv_net_dummy.o core/nv_net.o

.PHONY: all clean

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean
