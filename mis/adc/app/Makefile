# SPDX-License-Identifier: GPL-2.0

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2

BINDIR := bin
PROGS   := nv_test_adc nv_test_adc_sysfs

.PHONY: all clean

all: $(addprefix $(BINDIR)/,$(PROGS))

$(BINDIR):
	mkdir -p $@

$(BINDIR)/nv_test_adc: nv_test_adc.c | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BINDIR)/nv_test_adc_sysfs: nv_test_adc_sysfs.sh | $(BINDIR)
	cp $< $@
	chmod +x $@

clean:
	rm -rf $(BINDIR)
