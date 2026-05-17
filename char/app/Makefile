# SPDX-License-Identifier: GPL-2.0
# User-space tests for char driver (make -C char/app)

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2
LDFLAGS ?=

BINDIR := bin
PROGS  := nv_test_echo nv_test_demo nv_test_misc nv_test_plat

.PHONY: all clean install

all: $(addprefix $(BINDIR)/,$(PROGS))

$(BINDIR):
	mkdir -p $@

$(BINDIR)/%: %.c | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -rf $(BINDIR)

install: all
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(addprefix $(BINDIR)/,$(PROGS)) $(DESTDIR)/usr/local/bin/
