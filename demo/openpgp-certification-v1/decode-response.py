#!/usr/bin/env python3

from pathlib import Path
import hashlib
import subprocess
import sys

PREFIX = "KC1|OP=PGP_CERT_RESULT|CERT="
OUT = Path("/tmp/openpgp-approved-cert-from-qr.sig")


def parse_one_packet(data):
    if not data:
        raise ValueError("empty OpenPGP object")

    off = 0
    ctb = data[off]
    off += 1

    if not (ctb & 0x80):
        raise ValueError("invalid packet CTB")

    if ctb & 0x40:
        tag = ctb & 0x3F
        first = data[off]
        off += 1

        if first < 192:
            length = first
        elif first <= 223:
            length = ((first - 192) << 8) + data[off] + 192
            off += 1
        elif first == 255:
            length = int.from_bytes(data[off:off + 4], "big")
            off += 4
        else:
            raise ValueError("partial body length unsupported")
    else:
        tag = (ctb >> 2) & 0x0F
        lt = ctb & 3

        if lt == 0:
            length = data[off]
            off += 1
        elif lt == 1:
            length = int.from_bytes(data[off:off + 2], "big")
            off += 2
        elif lt == 2:
            length = int.from_bytes(data[off:off + 4], "big")
            off += 4
        else:
            raise ValueError("indeterminate packet length unsupported")

    end = off + length

    if end != len(data):
        raise ValueError("response is not exactly one OpenPGP packet")

    return tag


if len(sys.argv) != 2:
    raise SystemExit(
        "usage: decode-response.py /path/to/response-image.jpg"
    )

image = Path(sys.argv[1])

result = subprocess.run(
    ["zbarimg", "--raw", str(image)],
    check=True,
    stdout=subprocess.PIPE,
    text=True,
)

payload = result.stdout.strip()

if not payload.startswith(PREFIX):
    raise SystemExit("Unexpected QR response type")

hex_data = payload[len(PREFIX):]

if not hex_data or len(hex_data) % 2:
    raise SystemExit("Malformed certification hex")

try:
    cert = bytes.fromhex(hex_data)
except ValueError:
    raise SystemExit("Certification result is not valid hex")

tag = parse_one_packet(cert)

if tag != 2:
    raise SystemExit(f"Expected OpenPGP Signature packet tag 2, got {tag}")

OUT.write_bytes(cert)

print(f"Recovered response: {len(cert)} bytes")
print(f"OpenPGP packet tag: {tag}")
print(f"SHA256: {hashlib.sha256(cert).hexdigest()}")
print(f"Wrote {OUT}")
print("QR RETURN: PASS")
