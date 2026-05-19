#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
# SDIO test: load nv_sdio_demo and run sysfs test

set -e

MOD=nv_sdio_demo
SUBSYS=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
KO="${SUBSYS}/driver/${MOD}.ko"
APP="${SUBSYS}/app/bin/nv_test_sdio"

if [ "$(id -u)" -ne 0 ]; then
	echo "Run as root: sudo $0" >&2
	exit 1
fi

if [ ! -f "$KO" ]; then
	echo "Build first: make -C $SUBSYS" >&2
	exit 1
fi

if lsmod | grep -q "^${MOD} "; then
	rmmod "$MOD" 2>/dev/null || true
fi

insmod "$KO"
sleep 0.2

if [ -x "$APP" ]; then
	"$APP"
else
	echo "FAIL: $APP not found" >&2
	rmmod "$MOD" 2>/dev/null || true
	exit 1
fi

rmmod "$MOD"
echo "Done."
