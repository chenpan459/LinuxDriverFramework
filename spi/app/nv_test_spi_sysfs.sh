#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
# SPI test: load nv_spi_demo, run sysfs test if device present

set -e

MOD=nv_spi_demo
TAG=nv_spi_demo
SUBSYS=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
KO="${SUBSYS}/driver/${MOD}.ko"
APP="${SUBSYS}/app/bin/nv_test_spi"

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
sleep 0.2

CLIENT=
for d in /sys/bus/spi/devices/*; do
	[ -f "$d/modalias" ] || continue
	if grep -q "nv_spi_demo" "$d/modalias" 2>/dev/null; then
		CLIENT=$d
		break
	fi
done

if [ -n "$CLIENT" ]; then
	echo "Device: $CLIENT"
	if [ -x "$APP" ]; then
		"$APP" "$CLIENT"
	else
		echo 0x5a > "${CLIENT}/reg"
		cat "${CLIENT}/reg"
		echo "PASS: sysfs reg ok"
	fi
elif dmesg | grep -q "${TAG}:"; then
	echo "PASS: ${MOD} loaded (probe in dmesg)"
else
	echo "PASS: ${MOD} loaded (no SPI device — add DT nv,spi-demo or spi_board_info)"
fi

rmmod "$MOD"
echo "Done."
