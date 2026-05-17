# SPDX-License-Identifier: GPL-2.0

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2

BINDIR := bin
PROGS   := nv_test_spi nv_test_spi_sysfs

.PHONY: all clean

all: $(addprefix $(BINDIR)/,$(PROGS))

$(BINDIR):
	mkdir -p $@

$(BINDIR)/nv_test_spi: nv_test_spi.c | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BINDIR)/nv_test_spi_sysfs: nv_test_spi_sysfs.sh | $(BINDIR)
	cp $< $@
	chmod +x $@

clean:
	rm -rf $(BINDIR)
