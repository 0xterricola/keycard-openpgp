# Keycard Shell OpenPGP Demo — macOS

This is a short recording runbook for demonstrating the Keycard Shell `CREATE_IDENTITY` OpenPGP flow on macOS.

It is intentionally narrower than the full host E2E README: this file is for staging and recording a clean demo.

## What `CREATE_IDENTITY` means

`CREATE_IDENTITY` does **not** import somebody else's OpenPGP key and it does not change the Shell's private key.

The Shell already has a private key derived from its own fixed OpenPGP derivation path. From that private key it can derive the matching public key.

The purpose of `CREATE_IDENTITY` is to turn that Shell-derived key into a proper public OpenPGP identity certificate.

Conceptually:

```text
Shell-derived private key
        │
        └── derives matching public key
                    │
                    + requested UID
                      "Keycard Test <keycard@example.com>"
                    │
                    + self-certification made by
                      the same Shell private key
                    ↓
             OpenPGP certificate
```

The returned certificate contains the public side of that identity:

```text
Public Key packet
User ID packet
Self-certification Signature packet
```

The private key never leaves the Shell.

The self-certification means, in effect:

> the holder of the private key corresponding to this public key approved this UID binding.

After the host saves the returned certificate, other software can use its public key and fingerprint to verify future signatures made by the same Shell-derived private key.

This is different from the earlier external-certification experiment, where the Shell certified another party's existing public key and UID. In this demo, the Shell is establishing **its own** OpenPGP identity.

That makes `CREATE_IDENTITY` the foundation for later operations such as:

```text
SIGN_MESSAGE
    → sign an approved statement with this same Shell-derived key

external certification
    → use this Shell identity to certify another OpenPGP key/UID
```

## Demo goal

Show the complete flow:

```text
host asks the Shell to create an OpenPGP identity
        ↓
request contains UID + creation time
        ↓
request QR
        ↓
Keycard Shell trusted review
        ↓
Shell derives its own OpenPGP public key
        ↓
Shell binds the requested UID to that key
with a self-certification
        ↓
complete public OpenPGP certificate
        ↓
animated UR:BYTES response
        ↓
host reconstructs and saves certificate
        ↓
GnuPG verifies the self-signature
        ↓
PASS
```

The trusted review on the current beta3 firmware should show:

```text
UID
Keycard Test <keycard@example.com>

Unix time
<Unix timestamp>

Fingerprint
<40 hex characters>
```

Note: human-readable UTC rendering has been implemented in the newer OpenPGP orchestration branch, but it is not present on the beta3 firmware used for this demo.

---

# 1. Pre-demo setup

Use the existing host E2E directory:

```bash
cd ~/Developer/keycard-openpgp/experiments/shell-port/host-e2e
```

Confirm the required tools are available:

```bash
node --version
npm --version
qrencode --version
gpg --version
zbarimg --version
ffmpeg -version | head -1
```

Install JavaScript dependencies if needed:

```bash
npm ci
```

For the cleanest recording, close unrelated terminal windows and clear old demo files:

```bash
rm -f   /tmp/openpgp-create-identity.ur   /tmp/openpgp-create-identity.png   /tmp/openpgp-response-frames.txt   /tmp/openpgp-response-frames-unique.txt   /tmp/openpgp-created-identity.pgp

rm -rf   /tmp/openpgp-frames   /tmp/keycard-openpgp-gnupg
```

---

# 2. Generate the request

Generate the request immediately before recording so the creation time is easy to sanity-check on the Shell:

```bash
cd ~/Developer/keycard-openpgp/experiments/shell-port/host-e2e

TS=$(date +%s)

node generate-request.mjs   "Keycard Test <keycard@example.com>"   "$TS"
```

Expected final line:

```text
REQUEST SELF-CHECK: PASS
```

The generated QR is:

```text
/tmp/openpgp-create-identity.png
```

Open it:

```bash
open /tmp/openpgp-create-identity.png
```

Leave the QR visible and large enough for the Shell camera to scan.

---

# 3. Record the Shell request flow

On the Shell:

```text
Extras
  → OpenPGP
```

Scan the request QR.

The trusted review on this firmware should show the same request information:

```text
UID
Keycard Test <keycard@example.com>

Unix time
<Unix timestamp>

Fingerprint
<OpenPGP fingerprint>
```

For the recording, pause briefly on each page so the values are readable.

Approve the request.

The Shell should create the OpenPGP identity certificate and then display an animated `UR:BYTES` response QR.

Keep that animated response QR open.

---

# 4. Capture the animated response on macOS

List available cameras:

```bash
ffmpeg -f avfoundation -list_devices true -i "" 2>&1 |   sed -n '/AVFoundation video devices:/,/AVFoundation audio devices:/p'
```

If the MacBook camera is device `0`, prepare the frame directory:

```bash
rm -rf /tmp/openpgp-frames
mkdir -p /tmp/openpgp-frames
```

Hold the Shell steady in front of the MacBook camera with the animated response QR visible.

Capture 30 seconds and extract 15 frames per second:

```bash
ffmpeg   -f avfoundation   -framerate 30   -i "0:none"   -t 30   -vf "fps=15"   /tmp/openpgp-frames/frame-%05d.png
```

If the MacBook camera has a different device number, replace `0`.

---

# 5. Extract unique UR fragments

Create a fresh fragment file:

```bash
: > /tmp/openpgp-response-frames.txt
```

Run ZBar over the captured images:

```bash
for f in /tmp/openpgp-frames/*.png; do
  zbarimg     --set '*.disable'     --set 'qrcode.enable'     --raw     "$f"     2>/dev/null >> /tmp/openpgp-response-frames.txt || true
done
```

Keep only unique `UR:BYTES` fragments:

```bash
awk '/^[Uu][Rr]:[Bb][Yy][Tt][Ee][Ss]\// && !seen[$0]++'   /tmp/openpgp-response-frames.txt   > /tmp/openpgp-response-frames-unique.txt
```

Check the count:

```bash
wc -l /tmp/openpgp-response-frames-unique.txt
```

Optionally inspect the first few:

```bash
head -5 /tmp/openpgp-response-frames-unique.txt
```

Valid fragments begin with:

```text
UR:BYTES/
```

The fragments do not need to arrive in order. The Shell uses BC-UR fountain encoding.

---

# 6. Decode the Shell response

Run:

```bash
cd ~/Developer/keycard-openpgp/experiments/shell-port/host-e2e

node decode-response.mjs   /tmp/openpgp-response-frames-unique.txt
```

A successful result should look similar to:

```text
Recovered certificate: 236 bytes
Packet sequence:       6 -> 13 -> 2
UID:                   Keycard Test <keycard@example.com>
Wrote:                 /tmp/openpgp-created-identity.pgp
RESPONSE STRUCTURE CHECK: PASS
```

The exact certificate size may differ.

The important line is:

```text
RESPONSE STRUCTURE CHECK: PASS
```

Expected packet sequence:

```text
6  = Public Key
13 = User ID
2  = Signature
```

---

# 7. Inspect and verify with GnuPG

Inspect the certificate without importing it into the normal keyring:

```bash
gpg   --import-options show-only   --import   /tmp/openpgp-created-identity.pgp
```

For the final verification, use an isolated GnuPG home:

```bash
rm -rf /tmp/keycard-openpgp-gnupg
mkdir -m 700 /tmp/keycard-openpgp-gnupg
```

Import the certificate:

```bash
GNUPGHOME=/tmp/keycard-openpgp-gnupg   gpg --import /tmp/openpgp-created-identity.pgp
```

Show the fingerprint:

```bash
GNUPGHOME=/tmp/keycard-openpgp-gnupg   gpg --list-keys --with-fingerprint
```

Verify the certification signature:

```bash
GNUPGHOME=/tmp/keycard-openpgp-gnupg   gpg --check-sigs
```

Final success condition:

```text
[self-signature]
```

and:

```text
gpg: 1 good signature
```

---

# 8. Recording shot list

## Meta glasses / physical footage

Capture:

1. Shell in hand.
2. `Extras → OpenPGP`.
3. Shell scanning the request QR on the Mac.
4. Trusted review:
   - UID
   - raw Unix creation time
   - fingerprint
5. Approval.
6. Animated response QR appearing on the Shell.

Keep the Shell screen square to the glasses/camera and pause long enough for each review field to be readable.

## Mac screen footage

Capture:

1. Request generation ending in:

   ```text
   REQUEST SELF-CHECK: PASS
   ```

2. Response decoding ending in:

   ```text
   RESPONSE STRUCTURE CHECK: PASS
   ```

3. GnuPG fingerprint output.
4. Final:

   ```text
   gpg: 1 good signature
   ```

---

# 9. Suggested narration

Keep narration short.

Possible sequence:

```text
This is the CREATE_IDENTITY flow running on a physical Keycard Shell.

The Shell already has its own derived private key. CREATE_IDENTITY turns the public side of that key into a proper OpenPGP identity certificate by binding a requested user ID to it with a self-certification.

The host only sends the UID and creation timestamp. It does not send a private key or an existing public key for the Shell to adopt.

The Shell derives its matching OpenPGP public key, shows the UID, raw Unix creation time, and fingerprint in the trusted review, and only creates the certification signature after approval.

The private key never leaves the Shell.

The finished public certificate contains a public key packet, user ID packet, and self-certification signature packet, and is returned as an animated BC-UR QR.

Back on the host, the response is reconstructed and saved as a standard OpenPGP certificate.

Finally, GnuPG independently verifies the self-signature, proving that the UID is correctly bound to the Shell-derived key.
```

---

# 10. If QR capture fails

If no valid fragments were captured:

```bash
wc -l /tmp/openpgp-response-frames-unique.txt
```

If the count is `0`, repeat the camera capture while:

- keeping the Shell steadier
- reducing glare
- keeping the screen square to the camera
- moving the Shell slightly closer or farther away
- making the QR occupy more of the camera frame

If decoding reports:

```text
multipart UR response is incomplete
```

capture for longer, for example 60 seconds:

```bash
ffmpeg   -f avfoundation   -framerate 30   -i "0:none"   -t 60   -vf "fps=15"   /tmp/openpgp-frames/frame-%05d.png
```

Then repeat fragment extraction and decoding.

---

# Demo finish line

The demo is complete when all three are visible:

```text
REQUEST SELF-CHECK: PASS
RESPONSE STRUCTURE CHECK: PASS
gpg: 1 good signature
```

That demonstrates the complete macOS → Keycard Shell → self-certified OpenPGP identity → macOS verification path.

The important conceptual result is that the demo leaves you with a saved **public OpenPGP certificate** for the Shell-derived key. The Shell keeps the corresponding private key and can later use it for operations such as message signing.
