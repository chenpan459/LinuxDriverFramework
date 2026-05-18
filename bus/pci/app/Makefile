# SPDX-License-Identifier: GPL-2.0
# PCI tests: module load + dmesg check (no userspace device node)

BINDIR := bin
PROGS  := nv_test_pci

.PHONY: all clean

all: $(addprefix $(BINDIR)/,$(PROGS))

$(BINDIR):
	mkdir -p $@

$(BINDIR)/nv_test_pci: nv_test_pci.sh | $(BINDIR)
	cp $< $@
	chmod +x $@

clean:
	rm -rf $(BINDIR)
