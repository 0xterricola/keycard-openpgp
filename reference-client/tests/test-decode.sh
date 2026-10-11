#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CLI="$ROOT/reference-client/src/cli.mjs"
FIXTURES="$ROOT/artifacts/runs/2026-10-10-shell-openpgp"
HOST="$ROOT/experiments/shell-port/host-e2e"

echo "=== Identity response decoding ==="

(
  cd "$HOST"

  node --input-type=module - "$FIXTURES/certificate.pgp" <<'NODE'
import { readFileSync, writeFileSync } from "node:fs";
import { UR, UREncoder } from "@ngraveio/bc-ur";

const cert = readFileSync(process.argv[2]);
const encoder = new UREncoder(UR.fromBuffer(cert), 400, 0);

writeFileSync(
  "/tmp/keycard-identity-test.ur",
  encoder.nextPart() + "\n"
);
NODE
)

node "$CLI" decode-identity \
  --input /tmp/keycard-identity-test.ur

cmp "$FIXTURES/certificate.pgp" \
  /tmp/openpgp-created-identity.pgp

echo "PASS: Identity certificate round-trip"

echo
echo "=== Signature response decoding ==="

(
  cd "$HOST"

  node --input-type=module - "$FIXTURES/message.sig" <<'NODE'
import { readFileSync, writeFileSync } from "node:fs";
import { UR, UREncoder } from "@ngraveio/bc-ur";

const signature = readFileSync(process.argv[2]);
const encoder = new UREncoder(UR.fromBuffer(signature), 400, 0);

writeFileSync(
  "/tmp/keycard-signature-test.ur",
  encoder.nextPart() + "\n"
);
NODE
)

node "$CLI" decode-signature \
  --input /tmp/keycard-signature-test.ur

cmp "$FIXTURES/message.sig" \
  /tmp/openpgp-sign-message.sig

echo "PASS: Detached signature round-trip"
echo
echo "REFERENCE CLIENT QR DECODING TESTS: PASS"
