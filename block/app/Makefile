# SPDX-License-Identifier: GPL-2.0

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2

BINDIR := bin
PROGS  := nv_test_ram

.PHONY: all clean

all: $(addprefix $(BINDIR)/,$(PROGS))

$(BINDIR):
	mkdir -p $@

$(BINDIR)/%: %.c | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BINDIR)
