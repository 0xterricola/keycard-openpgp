#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../.."

RUN="artifacts/runs/2026-10-10-shell-openpgp"
CERT="$RUN/certificate.pgp"
MESSAGE="$RUN/message.txt"
SIGNATURE="$RUN/message.sig"
WRONG_KEY="artifacts/terricola-testtt.asc"

for file in "$CERT" "$MESSAGE" "$SIGNATURE" "$WRONG_KEY"; do
    if [[ ! -s "$file" ]]; then
        echo "FAIL: Missing or empty fixture: $file"
        exit 1
    fi
done

command -v rnp >/dev/null || {
    echo "FAIL: RNP is unavailable"
    exit 1
}

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

verify_case() {
    local name="$1"
    local expected_status="$2"
    local expected_pattern="$3"
    local key="$4"
    local message="$5"
    local signature="$6"
    local status=0
    local output

    output=$(rnp --verify \
        --keyfile "$key" \
        --source "$message" \
        "$signature" 2>&1) || status=$?

    echo "$output"

    if [[ "$expected_status" == "success" ]]; then
        if [[ "$status" -ne 0 ]]; then
            echo "FAIL: $name exited with status $status"
            exit 1
        fi
    else
        if [[ "$status" -eq 0 ]]; then
            echo "FAIL: $name unexpectedly succeeded"
            exit 1
        fi
    fi

    if ! grep -Fq "$expected_pattern" <<< "$output"; then
        echo "FAIL: $name did not produce expected diagnostic"
        echo "Expected: $expected_pattern"
        exit 1
    fi

    echo "PASS: $name"
    echo
}

echo "=== Positive verification ==="

verify_case \
    "Original message verified" \
    success \
    "Signature(s) verified successfully" \
    "$CERT" "$MESSAGE" "$SIGNATURE"

echo "=== Tampered message ==="

cp "$MESSAGE" "$TMP/tampered.txt"
printf 'TAMPERED\n' >> "$TMP/tampered.txt"

verify_case \
    "Tampered message rejected" \
    failure \
    "BAD signature" \
    "$CERT" "$TMP/tampered.txt" "$SIGNATURE"

echo "=== Wrong public key ==="

verify_case \
    "Wrong signing key rejected" \
    failure \
    "NO PUBLIC KEY" \
    "$WRONG_KEY" "$MESSAGE" "$SIGNATURE"

echo "=== Malformed signatures ==="

python3 - "$SIGNATURE" "$TMP" <<'PYTHON'
from pathlib import Path
import sys

original = Path(sys.argv[1]).read_bytes()
out = Path(sys.argv[2])

assert len(original) > 10

(out / "truncated.sig").write_bytes(original[:-10])

corrupted = bytearray(original)
corrupted[-1] ^= 0x01
(out / "corrupted.sig").write_bytes(corrupted)

(out / "empty.sig").write_bytes(b"")
PYTHON

verify_case \
    "Corrupted signature rejected" \
    failure \
    "BAD signature" \
    "$CERT" "$MESSAGE" "$TMP/corrupted.sig"

for variant in empty truncated; do
    status=0

    rnp --verify \
        --keyfile "$CERT" \
        --source "$MESSAGE" \
        "$TMP/${variant}.sig" \
        > "$TMP/${variant}.log" 2>&1 || status=$?

    if [[ "$status" -eq 0 ]]; then
        echo "FAIL: $variant signature accepted"
        exit 1
    fi

    if grep -Eq \
        'Good signature|Signature\(s\) verified successfully' \
        "$TMP/${variant}.log"; then
        echo "FAIL: $variant signature unexpectedly verified"
        exit 1
    fi

    echo "PASS: $variant signature rejected (exit $status)"
done

echo
echo "ALL DETACHED SIGNATURE TESTS PASSED"
