#!/usr/bin/env bash
set -euo pipefail
set +x

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

HOST_DIR="$SCRIPT_DIR"
THURIN_DIR="$REPO_ROOT/experiments/thurin"

CERT="${OPENPGP_CERT:-$HOST_DIR/fixtures/openpgp-created-identity.pgp}"
CAMERA="${CAMERA_DEVICE:-0}"
EVM_ADDRESS_INDEX="${EVM_ADDRESS_INDEX:-1}"
DEMO_VERBOSE="${DEMO_VERBOSE:-0}"
export DEMO_VERBOSE

REFERENCE_ETH_ADDRESS="0xB8A0f79E6d64c948E9F5ea32aD93647777915EBa"
REFERENCE_TX="https://etherscan.io/tx/0xddd7085ccc4f33865477eabaefd674c94f10a843cd0e531a1a25c89eac225076"
REFERENCE_IDENTITY="https://thurin.id/eth/$REFERENCE_ETH_ADDRESS"

if ! printf '%s' "$EVM_ADDRESS_INDEX" |
  grep -Eq '^[0-9]+$'
then
  echo "ERROR: EVM_ADDRESS_INDEX must be a non-negative integer"
  exit 1
fi

TMP="$(mktemp -d /tmp/keycard-thurin-e2e.XXXXXX)"

cleanup() {
  rm -rf "$TMP"
}

trap cleanup EXIT INT TERM

if [ ! -f "$CERT" ]; then
  echo "ERROR: missing stable OpenPGP certificate:"
  echo "$CERT"
  exit 1
fi

CERT_INFO="$(
  gpg \
    --import-options show-only \
    --with-colons \
    --import "$CERT" \
    2>/dev/null
)"

KEY_CREATION_TIME="$(
  printf '%s\n' "$CERT_INFO" |
    awk -F: '$1=="pub" {print $6; exit}'
)"

FINGERPRINT="$(
  printf '%s\n' "$CERT_INFO" |
    awk -F: '$1=="fpr" {print $10; exit}'
)"

if ! printf '%s' "$KEY_CREATION_TIME" | grep -Eq '^[0-9]+$'; then
  echo "ERROR: could not derive OpenPGP key creation time"
  exit 1
fi

if ! printf '%s' "$FINGERPRINT" | grep -Eq '^[0-9A-Fa-f]{40}$'; then
  echo "ERROR: could not derive OpenPGP fingerprint"
  exit 1
fi

capture_ur() {
  local name="$1"
  local seconds="$2"
  local ur_type="$3"

  local frames="$TMP/${name}-frames"
  local unique="$TMP/${name}-unique.txt"
  local ffmpeg_pid
  local payload
  local part
  local count
  local changed

  mkdir -p "$frames"
  : > "$unique"

  echo "Camera active — hold the response QR in view..." >&2

  ffmpeg \
    -nostdin \
    -hide_banner \
    -loglevel error \
    -f avfoundation \
    -framerate 30 \
    -i "${CAMERA}:none" \
    -t "$seconds" \
    -vf "fps=15" \
    "$frames/frame-%05d.png" \
    >/dev/null 2>&1 &

  ffmpeg_pid=$!

  while :; do
    changed=0

    for f in "$frames"/frame-*.png; do
      [ -f "$f" ] || continue
      [ -f "${f}.scanned" ] && continue

      payload="$(
        zbarimg \
          --set '*.disable' \
          --set 'qrcode.enable' \
          --raw \
          "$f" \
          2>/dev/null || true
      )"

      : > "${f}.scanned"

      if [ -z "$payload" ]; then
        continue
      fi

      while IFS= read -r part; do
        [ -n "$part" ] || continue

        if ! printf '%s\n' "$part" |
          grep -Eqi "^ur:${ur_type}/"
        then
          continue
        fi

        if ! grep -Fxiqx -- "$part" "$unique"; then
          printf '%s\n' "$part" >> "$unique"
          changed=1
        fi
      done <<EOF_PARTS
$payload
EOF_PARTS
    done

    if [ "$changed" -eq 1 ]; then
      count="$(wc -l < "$unique" | tr -d ' ')"

      if node "$HOST_DIR/check-ur-complete.mjs" \
        "$unique" \
        "$ur_type" \
        >/dev/null 2>&1
      then
        echo "QR capture complete ($count fragment(s)) — you can lower the Shell now." >&2

        kill "$ffmpeg_pid" 2>/dev/null || true
        wait "$ffmpeg_pid" 2>/dev/null || true

        printf '%s\n' "$unique"
        return 0
      fi

      echo "QR detected ($count fragment(s)); keep holding..." >&2
    fi

    if ! kill -0 "$ffmpeg_pid" 2>/dev/null; then
      break
    fi

    sleep 0.1
  done

  wait "$ffmpeg_pid" 2>/dev/null || true

  if node "$HOST_DIR/check-ur-complete.mjs" \
    "$unique" \
    "$ur_type" \
    >/dev/null 2>&1
  then
    count="$(wc -l < "$unique" | tr -d ' ')"
    echo "QR capture complete ($count fragment(s)) — you can lower the Shell now." >&2
    printf '%s\n' "$unique"
    return 0
  fi

  echo "ERROR: complete ${ur_type} UR was not captured" >&2
  return 1
}

echo
echo "========================================"
echo " KEYCARD → OPENPGP → THURIN → EVM"
echo "========================================"

echo
echo "=== 1. RUNTIME EVM ADDRESS ==="
echo "On the Shell open Ethereum address index $EVM_ADDRESS_INDEX."
read -r -p "When the address QR is visible, press Enter to start capture... "

OWNER="$(
  CAMERA_DEVICE="$CAMERA" \
    "$HOST_DIR/capture-evm-address.sh"
)"

if ! printf '%s' "$OWNER" |
  grep -Eq '^0x[0-9a-fA-F]{40}$'
then
  echo "ERROR: invalid Ethereum address returned by capture helper"
  exit 1
fi

echo
echo "CARD EVM ADDRESS:"
echo "$OWNER"
echo "ADDRESS QR: PASS"

echo
echo "=== 2. RUNTIME EIP4527 ACCOUNT METADATA ==="
echo "On the Shell open Connect → Ethereum."
read -r -p "Leave the crypto-hdkey QR showing, then press Enter... "

ACCOUNT_UR="$(
  capture_ur \
    eip4527-account \
    20 \
    crypto-hdkey
)"

if [ ! -s "$ACCOUNT_UR" ]; then
  echo "ERROR: no crypto-hdkey fragments captured"
  exit 1
fi

IFS=$'\t' read -r EVM_PATH EVM_XFP < <(
  node "$HOST_DIR/decode-eip4527-account.mjs" \
    "$ACCOUNT_UR" \
    "$EVM_ADDRESS_INDEX"
)

echo
echo "EVM SIGNING PATH:"
echo "$EVM_PATH"

echo "SOURCE FINGERPRINT:"
if [ "$DEMO_VERBOSE" = "1" ]; then
  echo "$EVM_XFP"
else
  echo "[captured from Keycard]"
fi

echo "EIP4527 METADATA: PASS"

echo
echo "=== 3. THURIN STATEMENT ==="

cd "$THURIN_DIR"

THURIN_STATEMENT_OUTPUT="$(
  ./node_modules/.bin/thurin attest \
    --network mainnet \
    --statement \
    --owner "$OWNER"
)"

STATEMENT="$(
  printf '%s\n' "$THURIN_STATEMENT_OUTPUT" \
    | grep -Eom1 \
      '^I control the Ethereum address: 0x[0-9a-fA-F]{40}$'
)"

if [ -z "$STATEMENT" ]; then
  echo "ERROR: could not extract Thurin statement"
  printf '%s\n' "$THURIN_STATEMENT_OUTPUT"
  exit 1
fi

echo "$STATEMENT"

echo
echo "=== 4. OPENPGP SIGN_MESSAGE ==="

cd "$HOST_DIR"

SIG_TS="$(date +%s)"

node generate-sign-message-request.mjs \
  "$STATEMENT" \
  "$KEY_CREATION_TIME" \
  "$SIG_TS"

open /tmp/openpgp-sign-message.png

echo
read -r -p \
  "Scan/sign on Shell. When response QR is showing, press Enter... "

PGP_UR="$(
  capture_ur \
    openpgp-signature \
    12 \
    bytes
)"

node decode-sign-message-response.mjs \
  "$PGP_UR"

echo
echo "=== 5. VERIFY OPENPGP BINDING ==="

cd "$THURIN_DIR"

OWNER="$OWNER" \
FINGERPRINT="$FINGERPRINT" \
CERT="$CERT" \
node --input-type=module <<'NODE'
import { readFileSync } from "node:fs";
import {
  verifyAttestation
} from "@thurinlabs/identity-kit";

const result = await verifyAttestation({
  pgpPublicKey: new Uint8Array(
    readFileSync(process.env.CERT)
  ),
  pgpSignature: new Uint8Array(
    readFileSync(
      "/tmp/openpgp-sign-message.sig"
    )
  ),
  fingerprint: process.env.FINGERPRINT,
  ethAddress: process.env.OWNER,
});

if (!result.verified) {
  process.exit(1);
}

console.log("identity-kit verification: PASS");
NODE

echo "OPENPGP → EVM BINDING: PASS"

echo
echo "=== 6. THURIN AIR-GAP AUTHORIZATION ==="

SIGNOUT="$TMP/thurin-sign-out.json"

./node_modules/.bin/thurin attest \
  --network mainnet \
  --owner "$OWNER" \
  --key-file "$CERT" \
  --statement-file /tmp/openpgp-sign-message.sig \
  --include-email \
  --authorize \
  --no-relay \
  --sign-out "$SIGNOUT"

echo
echo "THURIN SIGN-OUT: PASS"

echo
echo "=== 7. BUILD EIP-712 ETH-SIGN-REQUEST ==="

PARTS="$TMP/eth-sign-request.parts"
REQUEST_ID="$TMP/eth-sign-request-id.txt"

cd "$HOST_DIR"

node generate-thurin-eth-request.mjs \
  "$SIGNOUT" \
  "$EVM_PATH" \
  "$EVM_XFP" \
  "$OWNER" \
  "$PARTS" \
  "$REQUEST_ID"

QR_DIR="$TMP/eth-request-qrs"
mkdir -p "$QR_DIR"

i=0

while IFS= read -r part; do
  [ -n "$part" ] || continue

  printf -v frame \
    "%s/frame-%04d.png" \
    "$QR_DIR" \
    "$i"

  printf '%s' "$part" |
    qrencode \
      -s 6 \
      -m 2 \
      -o "$frame"

  i=$((i + 1))
done < "$PARTS"

if [ "$i" -eq 0 ]; then
  echo "ERROR: no eth-sign-request QR frames"
  exit 1
fi

REQUEST_GIF="$TMP/thurin-eip712-request.gif"

ffmpeg \
  -hide_banner \
  -loglevel error \
  -framerate 6 \
  -i "$QR_DIR/frame-%04d.png" \
  -loop 0 \
  "$REQUEST_GIF"

echo
echo "ETH-SIGN-REQUEST FRAMES: $i"

open -a Safari "$REQUEST_GIF"

echo
echo "On Shell open the normal QR transaction scanner."
echo "Scan the animated eth-sign-request."
echo "Review and approve the Thurin EIP-712 Attest request."

read -r -p \
  "When Shell's ETH_SIGNATURE response QR is showing, press Enter... "

echo
echo "=== 8. CAPTURE ETH_SIGNATURE ==="

ETH_SIG_UR="$(
  capture_ur \
    eth-signature \
    12 \
    eth-signature
)"

if [ ! -s "$ETH_SIG_UR" ]; then
  echo "ERROR: no eth-signature QR captured"
  exit 1
fi

EVM_SIGNATURE_FILE="$TMP/ethereum-signature.txt"

node "$HOST_DIR/decode-eth-signature.mjs" \
  "$ETH_SIG_UR" \
  "$REQUEST_ID" \
  > "$EVM_SIGNATURE_FILE"

if ! grep -Eq '^0x[0-9a-fA-F]{130}$' "$EVM_SIGNATURE_FILE"; then
  echo "ERROR: invalid Ethereum signature output"
  exit 1
fi

echo
echo "EVM SIGNATURE:"
echo "Ethereum signature captured: [raw signature hidden]"

echo
echo "=== 9. VERIFY EIP-712 SIGNER ==="

cd "$THURIN_DIR"

SIGNOUT="$SIGNOUT" \
EVM_SIGNATURE_FILE="$EVM_SIGNATURE_FILE" \
OWNER="$OWNER" \
node --input-type=module <<'NODE'
import { readFileSync } from "node:fs";
import {
  recoverTypedDataAddress
} from "viem";

const data = JSON.parse(
  readFileSync(
    process.env.SIGNOUT,
    "utf8"
  )
);

const td = data.typedData;

const recovered =
  await recoverTypedDataAddress({
    domain: td.domain,
    types: td.types,
    primaryType: td.primaryType,
    message: td.message,
    signature: readFileSync(
      process.env.EVM_SIGNATURE_FILE,
      "utf8"
    ).trim(),
  });

console.log("expected: ", process.env.OWNER);
console.log("recovered:", recovered);

if (
  recovered.toLowerCase() !==
  process.env.OWNER.toLowerCase()
) {
  throw new Error(
    "EIP-712 signature does not belong to runtime owner"
  );
}
NODE

echo "EIP-712 OWNER SIGNATURE: PASS"

echo
echo "=== 10. THURIN HANDOFF — NO MAINNET PUBLICATION ==="

THURIN_FINISH_OUT="$TMP/thurin-authorize-finish.txt"

if ! ./node_modules/.bin/thurin authorize finish \
  "$SIGNOUT" \
  --signature-file "$EVM_SIGNATURE_FILE" \
  > "$THURIN_FINISH_OUT" 2>&1
then
  echo "ERROR: Thurin authorize finish failed"
  sed -n '1,20p' "$THURIN_FINISH_OUT"
  exit 1
fi

if grep -Eqi 'https?://[^[:space:]]+' "$THURIN_FINISH_OUT"; then
  echo "THURIN HANDOFF LINK: [generated — hidden]"
else
  echo "THURIN AUTHORIZE FINISH: PASS"
fi

echo "MAINNET PUBLICATION: SKIPPED"

echo
echo "========================================"
echo " HARDWARE AUTHORIZATION VERIFIED"
echo "========================================"
echo "Live Ethereum index: $EVM_ADDRESS_INDEX"
echo "EVM owner:           $OWNER"
echo "PGP identity:        stable index 0"
echo "PGP fingerprint:     $FINGERPRINT"
echo "EVM path:            $EVM_PATH"

echo
echo "=== 11. PREVIOUS COMPLETED MAINNET REFERENCE ==="
echo "The live index-$EVM_ADDRESS_INDEX demo above was NOT published."
echo "The following is the previously completed Ethereum index-0 run."
echo
echo "Etherscan:"
echo "$REFERENCE_TX"
echo
echo "Thurin identity:"
echo "$REFERENCE_IDENTITY"

echo
echo "========================================"
echo " DEMO COMPLETE"
echo "========================================"
