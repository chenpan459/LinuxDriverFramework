#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
# CAN test: load nv_can_demo, bring up nvcan0, run SocketCAN test

set -e

MOD=nv_can_demo
IF=nvcan0
BITRATE=1000000
SUBSYS=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
KO="${SUBSYS}/driver/${MOD}.ko"
APP="${SUBSYS}/app/bin/nv_test_can"

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

ip link set "$IF" down 2>/dev/null || true
ip link set "$IF" up type can bitrate "$BITRATE"
sleep 0.2

if [ -x "$APP" ]; then
	"$APP" "$IF"
else
	echo "FAIL: $APP not found" >&2
	ip link set "$IF" down 2>/dev/null || true
	rmmod "$MOD" 2>/dev/null || true
	exit 1
fi

ip link set "$IF" down 2>/dev/null || true
rmmod "$MOD"
echo "Done."
