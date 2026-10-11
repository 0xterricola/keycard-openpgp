#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CLI="$ROOT/reference-client/src/cli.mjs"
FIXTURES="$ROOT/artifacts/runs/2026-10-10-shell-openpgp"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

cp "$FIXTURES/message.txt" "$TMP/message-tampered.txt"
printf 'tampered\n' >> "$TMP/message-tampered.txt"

echo "=== Valid hardware-generated signature ==="

node "$CLI" verify \
  --cert "$FIXTURES/certificate.pgp" \
  --file "$FIXTURES/message.txt" \
  --signature "$FIXTURES/message.sig"

echo
echo "=== Tampered message must fail ==="

if node "$CLI" verify \
  --cert "$FIXTURES/certificate.pgp" \
  --file "$TMP/message-tampered.txt" \
  --signature "$FIXTURES/message.sig"; then
  echo "FAIL: Tampered message was accepted"
  exit 1
fi

echo "PASS: Tampered message rejected"
echo
echo "REFERENCE CLIENT VERIFICATION TESTS: PASS"
