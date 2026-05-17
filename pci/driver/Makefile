# SPDX-License-Identifier: GPL-2.0
#
#   make
#   sudo insmod nv_pci_demo.ko
#   # With QEMU: -device edu

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
PWD        := $(shell pwd)

ccflags-y += -I$(src)/include

obj-m := nv_pci_demo.o
nv_pci_demo-y := modules/nv_pci_demo.o core/nv_pci.o

.PHONY: all clean

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean
