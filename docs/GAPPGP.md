# GapPGP

GapPGP is an open integration initiative for hardware-backed
OpenPGP signing using Keycard Shell.

Its goal is to connect applications with hardware signing
capabilities without exposing private signing keys to the host
application.

## Architecture

GapPGP separates the application requesting a signature from
the hardware performing the signing operation.

1. A host application prepares an OpenPGP signing request.
2. The request is encoded for QR-based transfer.
3. Keycard Shell processes the request with its OpenPGP firmware.
4. The hardware produces a signing response.
5. The host application decodes and verifies the response.
6. The application incorporates the signature into its workflow.

The underlying protocol uses versioned CBOR requests, QR framing,
and documented signing responses.

GapPGP does not introduce a new cryptographic standard.

## Implementations

### Keycard Shell

Hardware signing implementation supporting OpenPGP operations.

### Reference client

Experimental request-generation and response-decoding tools
maintained in the keycard-openpgp repository.

### GapPGP for Thunderbird

Experimental Thunderbird extension demonstrating hardware-backed
OpenPGP signatures and signed PGP/MIME email.

Repository:
https://github.com/0xterricola/gappgp-thunderbird

## Interoperability

Hardware-backed signatures have been tested with GnuPG,
RNP/Botan, and OpenPGP.js under documented compatibility
constraints.

## Security model

Private signing keys remain on the hardware signing device.

The host is responsible for constructing the correct signing
payload, validating the selected identity, and verifying the
returned signature.

A valid signature does not independently establish that the
user approved the intended content. Request review and identity
verification remain important security boundaries.

## Current limitations

GapPGP remains experimental.

The Thunderbird implementation is not a complete production
OpenPGP signing solution.

Native Thunderbird WebExtension camera acquisition remains
blocked in tested builds. External QR capture and manual
response transfer have been demonstrated.

## Related work

- [Keycard OpenPGP repository](https://github.com/0xterricola/keycard-openpgp)
- [GapPGP Thunderbird integration](https://github.com/0xterricola/gappgp-thunderbird)
- [Keycard Shell firmware](https://github.com/keycard-tech/keycard-shell)
