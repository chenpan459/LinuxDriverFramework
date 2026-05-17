# SPDX-License-Identifier: GPL-2.0

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
PWD        := $(shell pwd)

ccflags-y += -I$(src)/include

obj-m += nv_char_echo.o
obj-m += nv_char_demo.o
obj-m += nv_char_plat.o
obj-m += nv_char_misc_echo.o
obj-m += nv_char_selftest.o

nv_char_echo-y := examples/nv_char_echo.o core/nv_char.o
nv_char_demo-y := examples/nv_char_demo.o core/nv_char.o
nv_char_plat-y := examples/nv_char_plat.o core/nv_char.o
nv_char_misc_echo-y := examples/nv_char_misc_echo.o core/nv_char.o
nv_char_selftest-y := examples/nv_char_selftest.o core/nv_char.o

.PHONY: all clean

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean

# Tracepoints (when CONFIG_TRACING=y):
#   sudo cat /sys/kernel/tracing/events/nv_char/enable
#   echo 1 | sudo tee /sys/kernel/tracing/events/nv_char/enable
