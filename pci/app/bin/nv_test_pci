#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

set -e

MOD=nv_pci_demo
TAG=nv_pci_demo
SUBSYS=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
KO="${SUBSYS}/driver/${MOD}.ko"

if [ "$(id -u)" -ne 0 ]; then
	echo "Run as root: sudo $0" >&2
	exit 1
fi

if [ ! -f "$KO" ]; then
	echo "Build driver first: make -C ${SUBSYS}/driver" >&2
	exit 1
fi

dmesg -C 2>/dev/null || true

if lsmod | grep -q "^${MOD} "; then
	rmmod "$MOD" 2>/dev/null || true
fi

insmod "$KO"
sleep 0.2

if dmesg | grep -q "${TAG}:"; then
	echo "PASS: ${MOD} loaded, probe message in dmesg"
	PROBE=1
else
	echo "PASS: ${MOD} loaded (no matching PCI device — probe skipped)"
	PROBE=0
fi

rmmod "$MOD"

if [ "$PROBE" -eq 1 ] && dmesg | grep -qi "error\|oops"; then
	echo "FAIL: errors in dmesg" >&2
	exit 1
fi

exit 0
