#!/usr/bin/env python3

from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

ARMORED_TARGET = ROOT / "experiments/certification/thurin-labs-public.asc"
SIGNER_PUBLIC = ROOT / "artifacts/terricola-testtt.asc"

TARGET_BUNDLE = Path("/tmp/thurin-cert-target.pgp")
QR_CERT = Path("/tmp/openpgp-approved-cert-from-qr.sig")

COMBINED = Path("/tmp/thurin-labs-from-qr-certified.pgp")
GNUPGHOME = Path("/tmp/keycard-qr-final-gnupg")

TARGET_FINGERPRINT = "08B9374FDFBEC67EFFA24E669D3D86E35361EF7B"


def parse_packets(data):
    packets = []
    off = 0

    while off < len(data):
        start = off
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
                raise ValueError("partial lengths unsupported")
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
                raise ValueError("indeterminate length unsupported")

        end = off + length

        if end > len(data):
            raise ValueError("truncated packet")

        packets.append({
            "tag": tag,
            "start": start,
            "end": end,
            "raw": data[start:end],
        })

        off = end

    return packets


if not TARGET_BUNDLE.exists():
    raise SystemExit(
        "Missing /tmp/thurin-cert-target.pgp; run generate-request.py first"
    )

if not QR_CERT.exists():
    raise SystemExit(
        "Missing QR-recovered signature; run decode-response.py first"
    )

with tempfile.NamedTemporaryFile() as tmp:
    with ARMORED_TARGET.open("rb") as src:
        subprocess.run(
            ["gpg", "--dearmor"],
            stdin=src,
            stdout=tmp,
            check=True,
        )

    tmp.flush()
    full = Path(tmp.name).read_bytes()

target = TARGET_BUNDLE.read_bytes()
cert = QR_CERT.read_bytes()

full_packets = parse_packets(full)
target_packets = parse_packets(target)
cert_packets = parse_packets(cert)

if [p["tag"] for p in target_packets] != [6, 13, 2]:
    raise SystemExit("Unexpected target bundle structure")

if len(cert_packets) != 1 or cert_packets[0]["tag"] != 2:
    raise SystemExit("QR result is not exactly one OpenPGP Signature packet")

if full_packets[0]["raw"] != target_packets[0]["raw"]:
    raise SystemExit("Target primary key does not match published certificate")

matches = []

for i in range(len(full_packets) - 1):
    if (
        full_packets[i]["raw"] == target_packets[1]["raw"]
        and full_packets[i + 1]["raw"] == target_packets[2]["raw"]
    ):
        matches.append(i)

if len(matches) != 1:
    raise SystemExit(
        f"Expected exactly one target UID/self-cert match, got {len(matches)}"
    )

i = matches[0]
insert_at = full_packets[i + 1]["end"]

combined = full[:insert_at] + cert + full[insert_at:]
COMBINED.write_bytes(combined)

print(f"QR certification inserted: {len(cert)} bytes")
print(f"Final certificate: {len(combined)} bytes")
print(f"Wrote {COMBINED}")
print("ASSEMBLY: PASS")

shutil.rmtree(GNUPGHOME, ignore_errors=True)
GNUPGHOME.mkdir(mode=0o700)

env = dict(**__import__("os").environ)
env["GNUPGHOME"] = str(GNUPGHOME)

subprocess.run(
    ["gpg", "--import", str(SIGNER_PUBLIC)],
    env=env,
    check=True,
)

subprocess.run(
    ["gpg", "--import", str(COMBINED)],
    env=env,
    check=True,
)

print()
print("=== GnuPG verification ===")

subprocess.run(
    ["gpg", "--check-sigs", TARGET_FINGERPRINT],
    env=env,
    check=True,
)
