# OpenPGP Certification Demo v1

## What problem this addresses

A hardware signing device normally asks you to approve an operation it cannot
show you. You press the button trusting that the host sent what it displayed.
If the host is compromised, the screen and the signature can disagree, and
nothing on the device would catch it.

This demo removes that gap for one operation. A network-disconnected device
receives the actual OpenPGP packets over QR, parses them itself, verifies the
User ID self-certification, derives the fingerprint independently, and shows
the identity it is about to certify. Only then does a physical button press
and a locally entered PIN allow the card to sign.

Nothing the host claims is trusted. The fingerprint on the display is derived
from the same bytes that get signed.

## What is demonstrated

A third-party OpenPGP certification of the published Thurin Labs public key
and User ID, signed by a hardware-backed key on a removable card, carried in
and out optically, and verified afterwards by stock GnuPG.

Video: https://www.youtube.com/watch?v=8V4hmqGCYLg
(also on X: https://x.com/0xterricola/status/2102559246776983671)

Step-by-step walkthrough: [OPENPGP_CERT_DEMO_STEPS.md](OPENPGP_CERT_DEMO_STEPS.md)

## How it works

The request QR transports one opaque OpenPGP packet bundle:

    primary Public-Key packet
    + selected User ID packet
    + that UID's self-certification Signature packet

Envelope:

    KC1|OP=PGP_CERT|CERT=<hex OpenPGP packet bundle>

The trusted device independently:

1. parses the OpenPGP object;
2. verifies the UID self-certification;
3. derives the target UID and fingerprint;
4. displays them for physical review;
5. requires physical APPROVE;
6. requests the NeoPGP PIN locally;
7. derives the certification digest using device-local RTC time;
8. signs with the removable NeoPGP card;
9. returns a standard OpenPGP Signature packet by QR.

Response envelope:

    KC1|OP=PGP_CERT_RESULT|CERT=<hex OpenPGP Signature packet>

The host reconstructs the returned Signature packet, inserts it beside the
exact UID/self-cert pair from the request, and verifies the resulting public
certificate with stock GnuPG.

Two properties are worth noting. The self-certification is verified *before*
the UID reaches the display, so an unauthenticated identity is never shown.
And a UID that exceeds the display budget, or contains characters the device
cannot render, is refused outright rather than truncated.

Core source snapshot: `204f83e`

## Demo helpers

Build the ARM signer:

    ./demo/openpgp-certification-v1/build-signer.sh

Generate the request bundle and QR:

    python3 demo/openpgp-certification-v1/generate-request.py

Decode a photographed response QR:

    python3 demo/openpgp-certification-v1/decode-response.py /tmp/sama-cert-response.jpg

Assemble the returned certification and verify with GnuPG:

    python3 demo/openpgp-certification-v1/assemble-and-verify.py

## Expected proof

The final GnuPG output includes:

    sig!         79BB391497E8E6D4 2026-09-22  terricola-testtt

and:

    gpg: 5 good signatures

## Requirements

Host:

- GnuPG
- Python 3
- qrencode
- zbarimg
- Lima

Embedded build environment:

- existing Buildroot Lima instance
- ARM hard-float cross compiler
- PC/SC
- OpenSSL/libcrypto
- libqrencode
- zbar

## Scope

The SAMA5D3 prototype is a development reference platform. This demo proves a
network-disconnected optical signing transaction; it is not a claim that the
development board itself is a hardened production air-gap device.
