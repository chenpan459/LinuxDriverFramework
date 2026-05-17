# SPDX-License-Identifier: GPL-2.0
#
#   make all      # 各子系统 driver + app
#   make driver   # 仅内核模块
#   make app      # 仅用户态应用
#   make clean

SUBDIRS := char block net pci usb i2c spi uart

.PHONY: all driver app clean $(SUBDIRS)

all: driver app

driver: $(SUBDIRS)
	@for d in $(SUBDIRS); do $(MAKE) -C $$d driver; done

app: $(SUBDIRS)
	@for d in $(SUBDIRS); do $(MAKE) -C $$d app; done

clean:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done

$(SUBDIRS):
	$(MAKE) -C $@
