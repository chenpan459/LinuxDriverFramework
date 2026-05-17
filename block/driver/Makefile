# SPDX-License-Identifier: GPL-2.0
# Out-of-tree build for block framework + example
#
#   make
#   sudo insmod nv_block_ram.ko
#   sudo dd if=/dev/zero of=/dev/nv_ram bs=4K count=1
#   sudo rmmod nv_block_ram

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
PWD        := $(shell pwd)

ccflags-y += -I$(src)/include

obj-m := nv_block_ram.o
nv_block_ram-y := modules/nv_block_ram.o core/nv_block.o

.PHONY: all clean

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean
