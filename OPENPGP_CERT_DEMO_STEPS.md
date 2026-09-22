# OpenPGP Third-Party Certification Demo

## What this demo does

This demo creates a **third-party OpenPGP certification** of the published
Thurin Labs public key and User ID using my own hardware-backed OpenPGP
signing key.

I do **not** have Thurin's private key.

I am certifying the binding between:

```text
Thurin Labs <hello@thurin.id>
```

and the published Thurin Labs public key.

The signing device does not trust a fingerprint or digest supplied by the
computer. The QR contains the actual OpenPGP public-key material, and the
trusted device:

- parses the OpenPGP object;
- verifies the UID self-certification;
- derives the fingerprint itself;
- shows the UID and fingerprint for physical review;
- derives the certification digest locally;
- requires physical approval;
- requires the NeoPGP PIN locally;
- performs the private-key operation on the removable card.

---

## Private local setup

Load the machine-specific values:

```bash
source ~/.config/keycard-openpgp/demo.env
```

The demo instructions use:

```text
$SAMA_IP
$SSH_KEY
```

The actual values are kept outside this repository and are not shown here.

---

# Pre-demo: Start from a reset board

Start with the SAMA freshly rebooted and Ethernet connected.

Nothing about the signing transaction has happened yet.

## Hardware check

Make sure:

- Ethernet is connected to the SAMA.
- The USB certification-request webcam is connected to the SAMA.
- The NeoPGP card reader is connected.
- The NeoPGP card is inserted.
- The trusted display is connected.
- The keypad is connected.

## Reset the SAMA

```bash
ssh -i "$SSH_KEY" \
  root@"$SAMA_IP" \
  'reboot'
```

Wait for the board to come back.

Confirm it is reachable:

```bash
ssh -i "$SSH_KEY" \
  root@"$SAMA_IP" \
  'echo "SAMA READY"'
```

Set the board clock before the demo:

```bash
NOW_UTC="$(date -u '+%Y-%m-%d %H:%M:%S')"

ssh -i "$SSH_KEY" \
  root@"$SAMA_IP" \
  "date -s '$NOW_UTC' && hwclock -w"
```

## Check the frozen demo source

```bash
cd ~/Developer/keycard-openpgp

git status --short
```

The working tree should be clean.

Optional confirmation:

```bash
git describe --tags --exact-match 2>/dev/null || git log -1 --oneline
```

## Build the frozen signer

```bash
./demo/openpgp-certification-v1/build-signer.sh
```

Expected result includes:

```text
/tmp/openpgp-cert-qr-sign: ELF 32-bit ... ARM ...
```

## Copy the signer to the SAMA

```bash
scp -i "$SSH_KEY" \
  /tmp/openpgp-cert-qr-sign \
  root@"$SAMA_IP":/tmp/openpgp-cert-qr-sign
```

At this point:

- the board has been reset;
- Ethernet is still connected;
- the signer has been freshly built;
- the signer binary is installed on the SAMA.

**Do not disconnect Ethernet yet.**

---

# 1. Show the published public key

## What to explain

> Here is the public key published by Thurin Labs.
>
> I saved that exact public key locally for this demo. First I'll show the
> armored public key itself, then I'll let GnuPG parse it and show its
> fingerprint.
>
> Later, after the signing device is disconnected from Ethernet, it will
> independently parse the OpenPGP object from the QR and derive this
> fingerprint itself. The host is not supplying the fingerprint as an
> authoritative value.
>
> I'm going to create a third-party OpenPGP certification of this key and
> its published User ID using my own hardware-backed signing key.

First show the public key on the Thurin Labs website.

Then show the exact local armored public key:

```bash
cd ~/Developer/keycard-openpgp

cat experiments/certification/thurin-labs-public.asc
```

The output should be an ASCII-armored OpenPGP public key beginning with:

```text
-----BEGIN PGP PUBLIC KEY BLOCK-----
```

and ending with:

```text
-----END PGP PUBLIC KEY BLOCK-----
```

Now let GnuPG parse that exact public key:

```bash
gpg --show-keys --with-fingerprint \
  experiments/certification/thurin-labs-public.asc
```

Check the primary-key fingerprint:

```text
08B9 374F DFBE C67E FFA2  4E66 9D3D 86E3 5361 EF7B
```

Selected User ID:

```text
Thurin Labs <hello@thurin.id>
```

This host-side fingerprint is only a reference for the human.

The QR generated in the next step contains the actual OpenPGP packet bundle,
not a trusted host-provided fingerprint. The disconnected SAMA device later
parses those OpenPGP bytes and derives the fingerprint independently.

The fingerprint shown on the trusted device can then be compared with the
fingerprint shown here from the published public key.

---

# 2. Generate a fresh certification QR

## What to explain

> Now I'm generating a fresh QR request directly from the published
> OpenPGP public key.
>
> The QR contains the actual OpenPGP Public-Key packet, User ID packet,
> and that User ID's self-certification Signature packet.
>
> It does not contain a trusted host-supplied fingerprint or digest.
> The signing device will derive those itself.

Generate the QR fresh from the public key:

```bash
python3 demo/openpgp-certification-v1/generate-request.py \
  && open /tmp/openpgp-cert-request.png
```

Expected output includes:

```text
OpenPGP bundle: 234 bytes
QR payload: 489 ASCII bytes
UID: Thurin Labs <hello@thurin.id>
Host UID field: ABSENT
Host fingerprint: ABSENT
Host digest: ABSENT
```

The request is generated from:

```text
experiments/certification/thurin-labs-public.asc
```

The fresh QR is written to:

```text
/tmp/openpgp-cert-request.png
```

---

# 3. Start the signer on the SAMA

## What to explain

> Before disconnecting the network, I need to start the trusted signing
> program on the embedded device.

Start the signer detached so it continues running after Ethernet is removed:

```bash
ssh -i "$SSH_KEY" \
  root@"$SAMA_IP" \
  'chmod +x /tmp/openpgp-cert-qr-sign;
   rm -f /tmp/openpgp-demo.log;
   nohup /tmp/openpgp-cert-qr-sign \
     >/tmp/openpgp-demo.log 2>&1 </dev/null &
   echo $! > /tmp/openpgp-demo.pid;
   echo "Signer PID: $(cat /tmp/openpgp-demo.pid)"'
```

Confirm the detached signer is still running:

```bash
ssh -i "$SSH_KEY" \
  root@"$SAMA_IP" \
  'PID=$(cat /tmp/openpgp-demo.pid);
   kill -0 "$PID" &&
   echo "Signer running: $PID"'
```

Also visually confirm that the SAMA display is asking for the certification QR.

**Only then disconnect Ethernet.**

---

# 4. Disconnect Ethernet

## What to explain

> The trusted signing program is already running locally on the device.
> I'm disconnecting Ethernet before the signing transaction.

Physically unplug the Ethernet cable from the SAMA.

From this point forward, the certification transaction crosses the device
boundary optically through QR codes.

The SAMA already has locally:

- the running signer;
- camera;
- trusted display;
- keypad;
- NeoPGP card and reader;
- device-local clock.

Do not use SSH or SCP again during the signing transaction.

---

# 5. Scan and review the certification request

## What to explain

> Now the disconnected device scans the actual OpenPGP object.
>
> It verifies the User ID self-certification and independently derives
> the User ID and public-key fingerprint.

Show the request QR:

```text
/tmp/openpgp-cert-request.png
```

to the USB camera connected to the SAMA.

The trusted display should show:

```text
Thurin Labs <hello@thurin.id>
```

and fingerprint:

```text
08B9374FDFBEC67EFFA24E669D3D86E35361EF7B
```

Compare this fingerprint with the fingerprint shown from the published public
key in Step 1.

**Do not approve until the trusted display is correct.**

---

# 6. Physically approve and sign

## What to explain

> The physical approval button is the consent step.
>
> After approval, the device derives the OpenPGP certification digest
> locally.
>
> The PIN is a separate step: it authorizes access to my hardware-backed
> signing key.

Press **APPROVE** on the trusted device.

Enter the NeoPGP PIN using the local keypad.

Press **APPROVE** again to submit the PIN.

The removable NeoPGP card performs the private-key operation.

The private key never leaves the card.

After signing, the SAMA displays a response QR.

The response contains a standard OpenPGP Signature packet inside:

```text
KC1|OP=PGP_CERT_RESULT|CERT=<OpenPGP Signature packet>
```

---

# 7. Capture and decode the response QR

## What to explain

> The signed OpenPGP certification is returned optically as another QR.
> The signing device is still disconnected from Ethernet.

Use the NexiGo webcam connected to the Mac:

```bash
imagesnap \
  -d "NexiGo N930AF FHD Webcam" \
  -w 5 \
  /tmp/sama-cert-response.jpg
```

Decode the response:

```bash
python3 demo/openpgp-certification-v1/decode-response.py \
  /tmp/sama-cert-response.jpg
```

Expected result:

```text
Recovered response: 119 bytes
OpenPGP packet tag: 2
QR RETURN: PASS
```

The recovered standard OpenPGP Signature packet is written to:

```text
/tmp/openpgp-approved-cert-from-qr.sig
```

---

# 8. Assemble the certified key and verify with GnuPG

## What to explain

> Finally, I'm taking the OpenPGP certification returned through the QR
> channel, attaching it to the exact User ID that was reviewed, and asking
> stock GnuPG to verify the result.

Run:

```bash
python3 demo/openpgp-certification-v1/assemble-and-verify.py
```

The final output should contain a valid certification from the hardware-backed
signing key:

```text
sig!         79BB391497E8E6D4 2026-09-22  terricola-testtt
```

and finish with:

```text
gpg: 5 good signatures
```

---

# Complete flow

```text
published OpenPGP public key
        ↓
fresh certification request QR
        ↓
network-disconnected trusted device
        ↓
parse and verify OpenPGP target
        ↓
derive UID and fingerprint locally
        ↓
physical review and approval
        ↓
local PIN
        ↓
hardware-backed signing
        ↓
standard OpenPGP certification packet
        ↓
response QR
        ↓
Mac decodes optical response
        ↓
stock GnuPG verifies the certification
```
