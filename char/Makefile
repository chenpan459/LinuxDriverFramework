# SPDX-License-Identifier: GPL-2.0
# Out-of-tree build for char framework + example
#
#   make
#   sudo insmod nv_char_echo.ko
#   echo hello > /dev/nv_echo && cat /dev/nv_echo
#   sudo rmmod nv_char_echo

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
PWD        := $(shell pwd)

ccflags-y += -I$(src)/include

obj-m := nv_char_echo.o
nv_char_echo-y := examples/nv_char_echo.o core/nv_char.o

.PHONY: all clean

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean
