#!/usr/bin/env bash
set -euo pipefail

REPO=/Users/lumos/Developer/keycard-openpgp
CC=/home/lumos.linux/buildroot-lima/output/host/bin/arm-buildroot-linux-gnueabihf-gcc

limactl shell default -- bash -lc "
set -euo pipefail
cd '$REPO'

SYSROOT=\"\$($CC -print-sysroot)\"

$CC -Wall -Wextra \
  -ffunction-sections \
  -fdata-sections \
  -Iembedded/sama5d3 \
  -I\"\$SYSROOT/usr/include/PCSC\" \
  embedded/sama5d3/thurin_attest_v1.c \
  embedded/sama5d3/openpgp_v4.c \
  embedded/sama5d3/qr_request.c \
  embedded/sama5d3/neopgp_sign.c \
  -Wl,--gc-sections \
  -lcrypto \
  -lpcsclite \
  -lqrencode \
  -lzbar \
  -o /tmp/thurin-attest-v1

file /tmp/thurin-attest-v1
"

limactl copy \
  default:/tmp/thurin-attest-v1 \
  /tmp/thurin-attest-v1

echo
echo "Built:"
echo "  /tmp/thurin-attest-v1"
