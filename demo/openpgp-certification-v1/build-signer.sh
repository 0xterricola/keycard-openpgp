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
  experiments/certification/openpgp_cert_qr_sign_test.c \
  embedded/sama5d3/openpgp_target.c \
  embedded/sama5d3/openpgp_v4.c \
  embedded/sama5d3/qr_request.c \
  embedded/sama5d3/neopgp_sign.c \
  -Wl,--gc-sections \
  -lcrypto \
  -lpcsclite \
  -lqrencode \
  -lzbar \
  -o /tmp/openpgp-cert-qr-sign

file /tmp/openpgp-cert-qr-sign
"

limactl copy \
  default:/tmp/openpgp-cert-qr-sign \
  /tmp/openpgp-cert-qr-sign

echo
echo "Built:"
echo "  /tmp/openpgp-cert-qr-sign"
