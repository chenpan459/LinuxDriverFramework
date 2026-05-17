#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
# Full I2C test: load driver, create i2c-stub client, run sysfs + optional i2cdev

set -e

MOD=nv_i2c_demo
CHIP=nv_i2c_demo
ADDR=0x50
SUBSYS=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
KO="${SUBSYS}/driver/${MOD}.ko"
APP="${SUBSYS}/app/bin/nv_test_i2c"

if [ "$(id -u)" -ne 0 ]; then
	echo "Run as root: sudo $0" >&2
	exit 1
fi

if [ ! -f "$KO" ]; then
	echo "Build first: make -C $SUBSYS" >&2
	exit 1
fi

modprobe i2c_dev 2>/dev/null || true
modprobe i2c_stub "chip_addr=${ADDR}" 2>/dev/null || modprobe i2c_stub 2>/dev/null || true

BUS=
for d in /sys/bus/i2c/devices/i2c-*; do
	[ -d "$d" ] || continue
	BUS=$(basename "$d")
	break
done

if [ -z "$BUS" ]; then
	echo "No I2C adapter found" >&2
	exit 1
fi

if lsmod | grep -q "^${MOD} "; then
	rmmod "$MOD" 2>/dev/null || true
fi

insmod "$KO"

DEVDIR="/sys/bus/i2c/devices/${BUS}/${CHIP}"
# new_device creates N-00xx node; remove stale
for n in /sys/bus/i2c/devices/*-*; do
	[ -f "$n/name" ] || continue
	if grep -qx "$CHIP" "$n/name" 2>/dev/null; then
		echo "$(basename "$n")" > "/sys/bus/i2c/devices/${BUS}/delete_device" 2>/dev/null || true
	fi
done

echo "${CHIP} ${ADDR}" > "/sys/bus/i2c/devices/${BUS}/new_device"
sleep 0.2

# find client dir (e.g. 0-0050)
CLIENT=
for n in /sys/bus/i2c/devices/*-*; do
	[ -f "$n/name" ] || continue
	if grep -qx "$CHIP" "$n/name" 2>/dev/null; then
		CLIENT=$n
		break
	fi
done

if [ -z "$CLIENT" ]; then
	echo "FAIL: could not create ${CHIP} at ${ADDR}" >&2
	rmmod "$MOD" 2>/dev/null || true
	exit 1
fi

echo "Device: $CLIENT"
echo 0x5a > "${CLIENT}/reg"
VAL=$(cat "${CLIENT}/reg")
echo "reg=$VAL"

if [ -x "$APP" ]; then
	"$APP" "$CLIENT"
else
	echo "0x5a" | grep -q "$VAL" || { echo "FAIL: reg mismatch"; exit 1; }
	echo "PASS: sysfs reg ok"
fi

echo "$(basename "$CLIENT")" > "/sys/bus/i2c/devices/${BUS}/delete_device"
rmmod "$MOD"
echo "Done."
