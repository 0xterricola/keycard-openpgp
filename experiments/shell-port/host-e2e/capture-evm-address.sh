#!/usr/bin/env bash
set -euo pipefail

CAMERA_DEVICE="${CAMERA_DEVICE:-0}"
TMP_DIR="$(mktemp -d /tmp/keycard-evm-address.XXXXXX)"
FFMPEG_PID=""

cleanup() {
  if [ -n "$FFMPEG_PID" ]; then
    kill "$FFMPEG_PID" 2>/dev/null || true
    wait "$FFMPEG_PID" 2>/dev/null || true
  fi
  rm -rf "$TMP_DIR"
}
trap cleanup EXIT INT TERM

echo "Camera active — hold the Keycard Shell Ethereum address QR in view" >&2

ffmpeg \
    -nostdin \
  -hide_banner \
  -loglevel error \
  -f avfoundation \
  -framerate 30 \
  -i "${CAMERA_DEVICE}:none" \
  -vf "fps=8" \
  "${TMP_DIR}/frame-%05d.png" \
  >/dev/null 2>&1 &

FFMPEG_PID=$!

for _ in $(seq 1 150); do
  for f in "$TMP_DIR"/frame-*.png; do
    [ -f "$f" ] || continue
    [ -f "${f}.scanned" ] && continue

    : > "${f}.scanned"

    payload="$(
      zbarimg \
        --set '*.disable' \
        --set 'qrcode.enable' \
        --raw \
        "$f" \
        2>/dev/null \
        | tr -d '\r\n' \
        || true
    )"

    if printf '%s' "$payload" | grep -Eq '^0x[0-9a-fA-F]{40}$'; then
      echo "Ethereum address captured — you can lower the Shell now." >&2
      printf '%s\n' "$payload"
      exit 0
    fi
  done

  sleep 0.1
done

echo "ERROR: no Ethereum address QR detected" >&2
exit 1
