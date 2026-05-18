#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
# UART test: load nv_uart_demo and run loopback app

set -e

MOD=nv_uart_demo
SUBSYS=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
KO="${SUBSYS}/driver/${MOD}.ko"
APP="${SUBSYS}/app/bin/nv_test_uart"

if [ "$(id -u)" -ne 0 ]; then
	echo "Run as root: sudo $0" >&2
	exit 1
fi

if [ ! -f "$KO" ]; then
	echo "Build first: make -C $SUBSYS" >&2
	exit 1
fi

dmesg -C 2>/dev/null || true

if lsmod | grep -q "^${MOD} "; then
	rmmod "$MOD" 2>/dev/null || true
fi

insmod "$KO"
sleep 0.3

if [ ! -c /dev/ttyNV0 ]; then
	echo "FAIL: /dev/ttyNV0 not created" >&2
	dmesg | tail -20
	rmmod "$MOD" 2>/dev/null || true
	exit 1
fi

if [ -x "$APP" ]; then
	"$APP"
else
	echo "FAIL: $APP not found" >&2
	exit 1
fi

rmmod "$MOD"
echo "Done."
