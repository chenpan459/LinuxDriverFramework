#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

set -e

MOD=nv_usb_demo
TAG=nv_usb_demo
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
else
	echo "PASS: ${MOD} loaded (no matching USB device — probe skipped)"
fi

rmmod "$MOD"
exit 0
