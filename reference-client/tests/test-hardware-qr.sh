#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
FIXTURES="$ROOT/artifacts/runs/2026-10-10-shell-openpgp"
HOST="$ROOT/experiments/shell-port/host-e2e"
CLI="$ROOT/reference-client/src/cli.mjs"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "=== Decode original hardware signing request ==="

(
  cd "$HOST"

  node --input-type=module - "$FIXTURES/request.ur" "$TMP/message.txt" <<'NODE'
import { readFileSync, writeFileSync } from "node:fs";
import { decode } from "cbor2";
import { URDecoder } from "@ngraveio/bc-ur";

const requestText = readFileSync(process.argv[2], "utf8").trim();
const decoder = new URDecoder();

decoder.receivePart(requestText);

if (!decoder.isComplete() || !decoder.isSuccess()) {
  throw new Error("Could not decode captured request");
}

const request = decode(decoder.resultUR().decodeCBOR());

if (!(request instanceof Map) ||
    request.get(1) !== 1 ||
    request.get(2) !== 2) {
  throw new Error("Unexpected OpenPGP request");
}

const message = Buffer.from(request.get(3));

if (message.toString("utf8") !==
    "Keycard Shell OpenPGP independent signing test") {
  throw new Error("Unexpected signed message");
}

if (message.length !== 46) {
  throw new Error("Unexpected message length");
}

writeFileSync(process.argv[3], message);

console.log("Request: valid SIGN_MESSAGE v1");
console.log("Message:", message.toString("utf8"));
NODE
)

echo
echo "=== Decode captured hardware QR response ==="

node "$CLI" decode-signature \
  --input "$FIXTURES/response.ur"

cmp \
  /tmp/openpgp-sign-message.sig \
  "$FIXTURES/signature.sig"

echo "PASS: Hardware QR response matches saved signature"

echo
echo "=== Independent GnuPG verification ==="

node "$CLI" verify \
  --cert "$FIXTURES/certificate.pgp" \
  --file "$TMP/message.txt" \
  --signature "$FIXTURES/signature.sig"

echo
echo "HARDWARE QR REGRESSION TEST: PASS"
