# SPDX-License-Identifier: GPL-2.0
# block — driver (kernel) + app (user-space)

.PHONY: all driver app clean

all: driver app

driver:
	$(MAKE) -C driver

app:
	$(MAKE) -C app

clean:
	$(MAKE) -C driver clean
	$(MAKE) -C app clean
