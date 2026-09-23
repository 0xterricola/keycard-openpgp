#!/usr/bin/env python3

from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

ARMORED = ROOT / "experiments/certification/thurin-labs-public.asc"
TARGET_UID = b"Thurin Labs <hello@thurin.id>"

BUNDLE_OUT = Path("/tmp/thurin-cert-target.pgp")
PAYLOAD_OUT = Path("/tmp/openpgp-cert-request.txt")
QR_OUT = Path("/tmp/openpgp-cert-request.png")


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
                raise ValueError("partial body lengths unsupported")
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
                raise ValueError("indeterminate packet lengths unsupported")

        body_start = off
        end = body_start + length

        if end > len(data):
            raise ValueError("truncated packet")

        packets.append({
            "tag": tag,
            "raw": data[start:end],
            "body": data[body_start:end],
        })

        off = end

    return packets


with tempfile.NamedTemporaryFile() as tmp:
    with ARMORED.open("rb") as src:
        subprocess.run(
            ["gpg", "--dearmor"],
            stdin=src,
            stdout=tmp,
            check=True,
        )

    tmp.flush()
    full = Path(tmp.name).read_bytes()

packets = parse_packets(full)

if not packets or packets[0]["tag"] != 6:
    raise SystemExit("Published certificate does not begin with a Public-Key packet")

match = None

for i in range(1, len(packets) - 1):
    if (
        packets[i]["tag"] == 13
        and packets[i]["body"] == TARGET_UID
        and packets[i + 1]["tag"] == 2
    ):
        match = i
        break

if match is None:
    raise SystemExit("Selected UID/self-cert pair not found")

bundle = (
    packets[0]["raw"]
    + packets[match]["raw"]
    + packets[match + 1]["raw"]
)

bundle_packets = parse_packets(bundle)

if [p["tag"] for p in bundle_packets] != [6, 13, 2]:
    raise SystemExit("Generated bundle does not contain exactly key + UID + self-cert")

payload = "KC1|OP=PGP_CERT|CERT=" + bundle.hex().upper()

for forbidden in ("UID=", "KEY=", "FP=", "DIGEST="):
    if forbidden in payload:
        raise SystemExit(f"Forbidden host-authoritative field present: {forbidden}")

BUNDLE_OUT.write_bytes(bundle)
PAYLOAD_OUT.write_text(payload + "\n")

subprocess.run(
    [
        "qrencode",
        "-l", "M",
        "-s", "6",
        "-m", "4",
        "-o", str(QR_OUT),
    ],
    input=payload.encode(),
    check=True,
)

print(f"OpenPGP bundle: {len(bundle)} bytes")
print(f"QR payload: {len(payload)} ASCII bytes")
print(f"UID: {TARGET_UID.decode()}")
print("Host UID field: ABSENT")
print("Host fingerprint: ABSENT")
print("Host digest: ABSENT")
print(f"Wrote {BUNDLE_OUT}")
print(f"Wrote {PAYLOAD_OUT}")
print(f"Wrote {QR_OUT}")
