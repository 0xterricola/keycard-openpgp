# Keycard Shell OpenPGP Protocol v1

Status: Experimental implementation documentation.

This document describes the existing Keycard Shell OpenPGP QR
integration protocol. It is intended to support independent
host applications without requiring Thurin Labs tooling.

It is not a security audit or a claim of universal OpenPGP
interoperability.

## 1. Overview

Keycard Shell exposes two OpenPGP operations:

- CREATE_IDENTITY (operation 1)
- SIGN_MESSAGE (operation 2)

Requests use versioned CBOR maps transported through UR:BYTES
QR codes.

Responses contain OpenPGP artifacts transported through
UR:BYTES QR codes.

The host does not supply a derivation path or private key.

## 2. Request transport

Host applications MUST encode the operation request as a CBOR
map with unsigned integer keys.

The resulting CBOR bytes are wrapped as a UR:BYTES payload.

Conceptually:

Host request map
    -> CBOR encoding
    -> UR:BYTES
    -> QR code
    -> Keycard Shell

The Shell extracts the inner CBOR bytes before parsing the
OpenPGP request.

## 3. Protocol version

Current version: 1

Unsupported versions are rejected.

## 4. CREATE_IDENTITY

Operation identifier: 1

CBOR map:

    {
      1: 1,
      2: 1,
      3: uid_bytes,
      4: creation_time
    }

Fields:

- 1: Protocol version (unsigned integer)
- 2: Operation identifier (unsigned integer)
- 3: User ID (CBOR byte string)
- 4: Creation timestamp (unsigned 32-bit Unix timestamp)

Constraints:

- UID length: 1–255 bytes.
- UID must contain printable ASCII (0x20–0x7E).
- Creation timestamp must be nonzero.

Successful response:

A binary OpenPGP v4 public certificate containing:

1. Public Key packet (tag 6)
2. User ID packet (tag 13)
3. Signature packet (tag 2)

The certificate is returned through UR:BYTES.

## 5. SIGN_MESSAGE

Operation identifier: 2

CBOR map:

    {
      1: 1,
      2: 2,
      3: message_bytes,
      4: key_creation_time,
      5: signature_creation_time
    }

Fields:

- 1: Protocol version (unsigned integer)
- 2: Operation identifier (unsigned integer)
- 3: Message (CBOR byte string)
- 4: Key creation timestamp (unsigned 32-bit Unix timestamp)
- 5: Signature creation timestamp (unsigned 32-bit Unix timestamp)

Constraints:

- Message must not be empty.
- Firmware maximum: 255 bytes.
- Current experimental JavaScript generator maximum: 104 bytes.
- Message must contain printable ASCII with LF or CRLF line endings.
- Bare carriage returns are rejected.
- Both timestamps must be nonzero.
- Signature creation time must not precede key creation time.

Canonical-text signatures normalize line endings to CRLF
before hashing.

Successful response:

A binary OpenPGP v4 detached Signature packet containing:

- Signature packet tag: 2
- Signature type: 0x01 (canonical text)
- Public-key algorithm: 19 (ECDSA)
- Curve: secp256k1
- Hash algorithm: 8 (SHA-256)

The signature is returned through UR:BYTES.

## 6. QR responses

Successful operations produce binary OpenPGP artifacts.

The artifact bytes are wrapped in a CBOR byte string and
transported as UR:BYTES.

Host applications decode the UR and recover the original
binary OpenPGP artifact.

Multipart animated QR responses must be reconstructed
before processing.

## 7. User approval

CREATE_IDENTITY displays:

- User ID
- Creation timestamp (UTC)
- OpenPGP fingerprint

SIGN_MESSAGE displays:

- Message text
- Signature timestamp (UTC)
- OpenPGP fingerprint

The Shell requires approval through its paged-text review
interface before the Keycard signs.

Cancellation prevents the operation from proceeding.

The derivation path is selected by the Shell rather than
supplied through host-controlled CBOR fields.

## 8. Validation and errors

The firmware rejects:

- Empty requests
- Unsupported protocol versions
- Unsupported operations
- Malformed CBOR
- Unexpected request structure
- Trailing CBOR bytes
- Empty or oversized payloads
- Invalid timestamps
- Signature timestamps preceding key creation
- Non-reviewable UID or message content

This version does not define a structured error response
schema for host applications.

## 9. Current limitations

- Experimental interface; not a finalized interoperability standard.
- SIGN_MESSAGE supports short, reviewable text, not arbitrary files.
- Host applications must implement UR:BYTES QR communication.
- Native Thunderbird integration has not been demonstrated.
- Git commit signing integration has not been demonstrated.
- Sequoia compatibility depends on secp256k1 support.
- OpenPGP.js verification was demonstrated with secp256k1
  explicitly permitted by configuration.
- secp256k1 OpenPGP identities may be linkable to Ethereum
  addresses derived from the same public key.

## 10. Reference implementation

Current experimental tooling:

- experiments/shell-port/host-e2e/generate-request.mjs
- experiments/shell-port/host-e2e/generate-sign-message-request.mjs
- experiments/shell-port/host-e2e/decode-response.mjs
- experiments/shell-port/host-e2e/decode-sign-message-response.mjs

Authoritative firmware implementation:

- keycard-shell/app/openpgp/openpgp_protocol.h
- keycard-shell/app/openpgp/openpgp_protocol.c
- keycard-shell/app/core/core_openpgp.c

## 11. Next steps

- Add reproducible CBOR and UR test vectors.
- Review protocol behavior against current Shell firmware.
- Build an independent reference client.
- Investigate release signing, Git commit signing,
  and Thunderbird integration.
