# SPDX-License-Identifier: GPL-2.0

BINDIR := bin
PROGS  := nv_test_usb

.PHONY: all clean

all: $(addprefix $(BINDIR)/,$(PROGS))

$(BINDIR):
	mkdir -p $@

$(BINDIR)/nv_test_usb: nv_test_usb.sh | $(BINDIR)
	cp $< $@
	chmod +x $@

clean:
	rm -rf $(BINDIR)
