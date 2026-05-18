# SPDX-License-Identifier: GPL-2.0
# Requires: linux/can.h (linux-libc-dev or libsocketcan-dev)

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2

BINDIR := bin
PROGS   := nv_test_can nv_test_can_sysfs

.PHONY: all clean

all: $(addprefix $(BINDIR)/,$(PROGS))

$(BINDIR):
	mkdir -p $@

$(BINDIR)/nv_test_can: nv_test_can.c | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BINDIR)/nv_test_can_sysfs: nv_test_can_sysfs.sh | $(BINDIR)
	cp $< $@
	chmod +x $@

clean:
	rm -rf $(BINDIR)
