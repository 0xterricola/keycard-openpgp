# Keycard Shell OpenPGP Verification

Date: 2026-10-10

## Certificate

Fingerprint:
07D8539F269583C160E333151763E15ED3E9286C

User ID:
Keycard Test <keycard@example.com>

Public-key algorithm: ECDSA
Curve: secp256k1
Hash: SHA-256
Certificate format: OpenPGP v4

## Independent verification

RNP: 0.18.1
Crypto backend: Botan 3.13

Command:

experiments/rnp-diagnostics/verify \
  artifacts/runs/2026-10-10-shell-openpgp/certificate.pgp

Observed result:

Certificate: 07D8539F269583C160E333151763E15ED3E9286C
User ID signatures: 1
Signature 0: VALID
Exit code: 0

Verification used rnp_signature_is_valid() with
RNP_SIGNATURE_REVALIDATE.

## Sequoia interoperability

Sequoia OpenPGP: 2.4.1
Crypto backend: Botan

Observed result:

SELF-SIGNATURE: FAILED
Unsupported elliptic curve: Unknown curve 1.3.132.0.10

This indicates unsupported secp256k1 verification
in the tested Sequoia configuration.

It does not invalidate RNP's successful verification.

## Scope and limitations

PASS: Certificate self-signature verification.
PASS: Detached message-signature verification (RNP, GnuPG, OpenPGP.js).
NOT TESTED: Cross-device verification.
NOT ESTABLISHED: Universal OpenPGP interoperability.

This test does not constitute a security audit.

## Detached message signature verification

Hardware: Keycard Shell
Signature format: OpenPGP v4 detached canonical text
Public-key algorithm: ECDSA secp256k1
Hash: SHA-256
Signature length: 119 bytes

### RNP 0.18.1 / Botan 3.13

Original message: VALID
Tampered message: INVALID
Regression suite: PASS

Command:

experiments/rnp-diagnostics/test-detached.sh

### GnuPG 2.5.22

Original message: Good signature
Original exit code: 0

Tampered message: BAD signature
Tampered exit code: 1

### OpenPGP.js 6.3.1

Configuration:

rejectCurves: new Set()

Original message: PASS
Tampered message: PASS (rejected: Signed digest did not match)
Exit code: 0

Command:

cd experiments/thurin
node test-shell-detached.mjs

### Sequoia OpenPGP 2.4.1 / Botan

Certificate self-signature verification:
Unsupported elliptic curve 1.3.132.0.10

Detached signature verification: NOT TESTED

## Interoperability limitations

These results demonstrate compatibility with the tested versions
and configurations. They do not establish universal OpenPGP
interoperability, production readiness, or security audit status.

## Canonical-text interoperability

Hardware-generated multiline OpenPGP v4 detached signature.

Message:
Keycard Shell OpenPGP
Canonical text test

RNP 0.18.1 / Botan 3.13:

LF representation: VALID (exit 0)
CRLF representation: VALID (exit 0)

Both representations successfully verify against the
same detached signature.

This demonstrates LF/CRLF normalization for the tested
canonical-text signing workflow.

## Hardened regression suite

Script: experiments/rnp-diagnostics/test-detached.sh

Results:
- Original detached signature: PASS
- Tampered message rejection: PASS
- Wrong-key rejection: PASS
- Corrupted ECDSA signature rejection: PASS
- Empty signature rejection: PASS
- Truncated signature rejection: PASS

Overall exit code: 0

Negative tests check expected diagnostics where available.
Empty and truncated signature tests require nonzero exit
status and absence of successful verification output.

Malformed fixtures are generated automatically in a
temporary directory and cleaned up after execution.

These tests validate RNP's verification behavior against
hardware-generated Keycard Shell fixtures. They do not
directly test Keycard Shell's input parser.
