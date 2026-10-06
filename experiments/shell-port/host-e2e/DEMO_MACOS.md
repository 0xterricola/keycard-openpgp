# Keycard Shell OpenPGP CREATE_IDENTITY Demo — macOS

`CREATE_IDENTITY` creates a public OpenPGP identity for the Shell’s own derived key by binding a UID to its public key with a self-certification.

This is the macOS runbook for demonstrating `CREATE_IDENTITY` on a physical Keycard Shell.

The goal is to show the complete hardware flow:

```text
CREATE_IDENTITY request
        ↓
Shell trusted review
        ↓
Shell creates its OpenPGP identity
        ↓
animated certificate QR
        ↓
host reconstructs certificate
        ↓
GnuPG verifies self-certification
```

## What CREATE_IDENTITY means

The Shell already has a private key derived from its fixed OpenPGP derivation path.

`CREATE_IDENTITY` does **not** import somebody else's key and does not replace the Shell's private key.

Instead:

```text
Shell-derived private key
        │
        └── derives matching public key
                    │
                    + requested UID
                    │
                    + self-certification
                    ↓
             OpenPGP certificate
```

The returned public certificate contains:

```text
Public Key packet
User ID packet
Self-certification Signature packet
```

The private key never leaves the Shell.

The self-certification proves that the holder of the corresponding private key approved the binding between this public key and this UID.

This establishes the Shell's own OpenPGP identity. Later operations such as `SIGN_MESSAGE` can use the same Shell-derived key.

---

# 1. SETUP

Do this first.

```bash
cd ~/Developer/keycard-openpgp/experiments/shell-port/host-e2e

rm -rf \
  /tmp/openpgp-frames \
  /tmp/keycard-openpgp-gnupg

rm -f \
  /tmp/openpgp-create-identity.ur \
  /tmp/openpgp-create-identity.png \
  /tmp/openpgp-response-frames.txt \
  /tmp/openpgp-response-frames-unique.txt \
  /tmp/openpgp-created-identity.pgp

mkdir -p /tmp/openpgp-frames
mkdir -m 700 /tmp/keycard-openpgp-gnupg
```


---

# 2. GENERATE REQUEST

Generate a fresh Unix creation timestamp and create the request:

```bash
TS=$(date +%s)

node generate-request.mjs \
  "Keycard Test <keycard@example.com>" \
  "$TS"
```

Show:

```text
REQUEST SELF-CHECK: PASS
```

Open the QR:

```bash
open /tmp/openpgp-create-identity.png
```

---

# 3. KEYCARD SHELL

On the Shell:

```text
Extras
  → OpenPGP
```

Scan the QR displayed on the Mac.

The current beta3 firmware should show:

```text
UID
Keycard Test <keycard@example.com>

Unix time
<creation timestamp>

Fingerprint
<OpenPGP fingerprint>
```

Review the displayed values.

Approve the request.

The Shell creates the OpenPGP certificate and displays an animated `UR:BYTES` response QR.

Keep the animated QR open.

---

# 4. CAPTURE THE ANIMATED RESPONSE

Hold the Shell steady in front of the MacBook camera.

Capture 12 seconds:

```bash
rm -rf /tmp/openpgp-frames
mkdir -p /tmp/openpgp-frames

ffmpeg \
  -f avfoundation \
  -framerate 30 \
  -pixel_format uyvy422 \
  -i "0:none" \
  -t 12 \
  -vf "fps=10,scale=720:-1" \
  /tmp/openpgp-frames/frame-%04d.png
```

This assumes the MacBook camera is AVFoundation device `0`, which is the device used in the tested setup.

---

# 5. EXTRACT AND DECODE THE RESPONSE

Run ZBar once across all captured frames and deduplicate the BC-UR fragments:

```bash
zbarimg \
  --set '*.disable' \
  --set 'qrcode.enable' \
  --raw \
  /tmp/openpgp-frames/*.png \
  2>/dev/null |
awk '/^[Uu][Rr]:[Bb][Yy][Tt][Ee][Ss]\// && !seen[$0]++' \
  > /tmp/openpgp-response-frames-unique.txt
```

Decode immediately:

```bash
node decode-response.mjs \
  /tmp/openpgp-response-frames-unique.txt
```

Show the result:

```text
Recovered certificate: 236 bytes
Packet sequence:       6 -> 13 -> 2
UID:                   Keycard Test <keycard@example.com>
Wrote:                 /tmp/openpgp-created-identity.pgp
RESPONSE STRUCTURE CHECK: PASS
```

The exact certificate size may differ.

The important results are:

```text
Packet sequence: 6 -> 13 -> 2
RESPONSE STRUCTURE CHECK: PASS
```

Where:

```text
6  = Public Key
13 = User ID
2  = Signature
```

---

# 6. FINAL GnuPG VERIFICATION

Skip redundant `show-only`, `--list-keys`, and packet-dump steps.

Import the reconstructed certificate into the isolated GnuPG home:

```bash
GNUPGHOME=/tmp/keycard-openpgp-gnupg \
  gpg --import /tmp/openpgp-created-identity.pgp
```

Verify the self-certification:

```bash
GNUPGHOME=/tmp/keycard-openpgp-gnupg \
  gpg --check-sigs
```

The final success condition is:

```text
gpg: 1 good signature
```

Once that appears, the end-to-end flow has passed.

---

# SPEED-RUN FLOW

The complete flow is:

```text
generate CREATE_IDENTITY request
        ↓
REQUEST SELF-CHECK: PASS
        ↓
open request QR
        ↓
Shell: Extras → OpenPGP
        ↓
scan
        ↓
show UID / Unix time / fingerprint
        ↓
approve
        ↓
animated response QR
        ↓
12-second camera capture
        ↓
ZBar extract
        ↓
decode
        ↓
6 -> 13 -> 2
        ↓
RESPONSE STRUCTURE CHECK: PASS
        ↓
GnuPG import
        ↓
GnuPG check-sigs
        ↓
gpg: 1 good signature
        ↓
END
```

---

---
