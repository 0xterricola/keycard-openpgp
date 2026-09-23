# OpenPGP Certification Demo v1

Frozen demonstration harness for the trusted OpenPGP key-certification flow.

Core source snapshot:

    204f83e

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

The SAMA5D3 prototype is a development reference platform. This demo proves a
network-disconnected optical signing transaction; it is not a claim that the
development board itself is a hardened production air-gap device.
