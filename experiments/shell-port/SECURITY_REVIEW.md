# Keycard Shell OpenPGP — Security Review

## Purpose

This document tracks the defensive security review of the Keycard Shell
OpenPGP implementation.

It is a living engineering review, not an independent third-party security
certification.

Findings remain in this document after remediation so that the review history,
fixes, and regression-test evidence remain visible.

## Review Scope

Upstream Keycard Shell PR stack:

- PR #225 — OpenPGP primitives
- PR #226 — versioned OpenPGP request protocol
- PR #227 — trusted identity creation and Shell integration

Current reviewed implementation snapshot:

- PR #225 head: `1212e05`
- PR #226 head: `8b5a6d2`
- PR #227 head: `3c1f9f0`

Primary data flow:

    untrusted host / QR
            ↓
    request parser
            ↓
    Shell-owned derivation policy
            ↓
    Keycard public-key export
            ↓
    OpenPGP key + fingerprint
            ↓
    exact UID + fingerprint display
            ↓
    physical approval
            ↓
    certification digest
            ↓
    Keycard signing
            ↓
    certificate construction
            ↓
    binding + self-cert verification
            ↓
    UR:BYTES response

## Primary Security Property

The Keycard must not sign materially different data from what the Shell
derived, displayed, and the user approved.

Additional properties:

- untrusted input must not select the Keycard derivation path
- untrusted input must not directly provide the public key
- untrusted input must not directly provide the fingerprint
- untrusted input must not directly provide the certification digest
- untrusted input must not directly provide the final signature
- cancel/reject must return before the Keycard signing operation
- malformed inputs must fail closed
- certificate output must be cryptographically verified before release

## Threat Model

Treat as attacker-controlled:

- QR contents
- CBOR request bytes
- UID bytes
- `creation_time`
- malformed, truncated, oversized, or unexpected requests
- repeated requests
- cancel/reject sequences
- unexpected card/APDU responses where applicable

Treat as trusted for the current design:

- Shell firmware
- Shell-owned derivation policy
- Shell display
- physical user approval
- Keycard private key and signing operation

## Finding Summary

| ID | Severity | Finding | Status |
|---|---|---|---|
| SR-001 | Medium | Host-controlled creation time is not semantically reviewed | Open — design decision required |
| SR-002 | Low | `keycard_cmd_sign()` fixed signing buffer lacks an explicit path-length bound | Open — hardening |
| SR-003 | Low | MPI verifier accepts non-canonical bit-length encodings | Open — hardening |
| SR-004 | Low | Generic OpenPGP helpers narrow some `size_t` lengths without explicit upper-bound rejection | Open — hardening |
| SR-005 | Informational | OpenPGP v4 / RFC 9580 compatibility position must remain explicit | Documented |
| SR-006 | Low | Inherited signature-response TLV parsing did not enforce logical APDU response bounds | Remediation submitted — [keycard-tech/keycard-shell#228](https://github.com/keycard-tech/keycard-shell/pull/228) |

No Critical or High-severity issue has been confirmed in the reviewed
`CREATE_IDENTITY` path at this checkpoint.

That statement is provisional until the remaining review and physical E2E
testing are complete.

---

## SR-001 — Host-Controlled Creation Time Is Not Semantically Reviewed

**Severity:** Medium
**Confidence:** High
**Status:** Open — design decision required

### Description

The OpenPGP request allows the host to provide `creation_time`.

That value becomes part of the OpenPGP primary-key packet and therefore affects
the v4 fingerprint. It is also included in the certification signature
metadata.

The current approval flow displays:

- exact UID
- derived fingerprint

but does not separately display the semantic creation timestamp.

### Impact

A host can request the same Keycard key and UID using different creation times,
causing distinct OpenPGP v4 fingerprints to be created from the same underlying
secret key.

This does not expose the private key and does not provide an arbitrary signing
oracle.

The concern is identity stability and trusted semantic review: host-controlled
metadata materially changes the resulting identity while the user sees only its
derived fingerprint.

### Candidate Remediation

Choose one explicit policy:

1. make creation time Shell-owned / persisted, or
2. allow a host-proposed creation time but display and approve it explicitly.

The final choice should be reviewed with the Keycard maintainers.

### Regression Tests

- [ ] different unapproved creation times cannot silently create identities
- [ ] approved creation time matches encoded primary-key creation time
- [ ] approved creation time matches signature creation-time metadata
- [ ] fingerprint produced after approval matches the final certificate

---

## SR-002 — Signing Buffer Needs Explicit Path-Length Bound

**Severity:** Low
**Confidence:** High
**Status:** Open — hardening

### Description

The shared Keycard signing helper uses a fixed signing buffer and copies:

    hash || derivation_path

into it.

An explicit check that:

    hash_len + path_len <= signing_buffer_size

should occur before the copies.

### Current Reachability

The current OpenPGP production path is five 32-bit components:

    m/43'/60'/1581'/5261136'/0

which serializes to 20 bytes.

With the current 32-byte ECDSA digest, the OpenPGP call uses approximately:

    32 + 20 = 52 bytes

and therefore does not approach the existing 104-byte signing buffer.

The finding is defensive hardening of the shared Keycard function rather than a
demonstrated overflow through the current OpenPGP production path.

### Remediation

Add an explicit bounds check in the shared Keycard signing helper before either
`memcpy`.

### Regression Tests

- [ ] normal OpenPGP path signs successfully
- [ ] maximum accepted path fits
- [ ] oversized path is rejected before copying
- [ ] rejected oversized path does not invoke signing

---

## SR-003 — Non-Canonical MPI Encodings Accepted by Verifier

**Severity:** Low
**Confidence:** High
**Status:** Open — hardening

### Description

The OpenPGP MPI reader derives the encoded byte count from the declared MPI bit
length and bounds-checks that byte count.

It does not currently verify that the declared bit length is the canonical bit
length of the encoded integer.

### Current Reachability

Shell-generated ECDSA `r` and `s` values are encoded canonically by the local
MPI encoder before being verified.

The current certificate self-verifier therefore normally receives
Shell-generated MPI encodings rather than arbitrary host-provided signature
packets.

### Remediation

Validate that:

- unused high bits are zero where required
- the declared MPI bit length matches the actual most-significant set bit
- zero and oversized scalar encodings remain rejected

### Regression Tests

- [ ] canonical `r` and `s` accepted
- [ ] incorrect declared bit length rejected
- [ ] leading-zero non-canonical encoding rejected
- [ ] oversized scalar rejected
- [ ] truncated MPI rejected

---

## SR-004 — Explicit Length-Narrowing Checks

**Severity:** Low
**Confidence:** High
**Status:** Open — hardening

### Description

Some reusable OpenPGP primitives narrow `size_t` values to protocol-sized
integer fields.

Examples include conversion of User ID length to a 32-bit OpenPGP field.

### Current Reachability

The current request protocol caps the UID at 255 bytes, so the production
identity flow cannot reach the large values needed for truncation.

### Remediation

Add explicit upper-bound checks before narrowing generic `size_t` lengths.

### Regression Tests

- [ ] maximum supported UID accepted
- [ ] oversized generic lengths rejected before integer conversion
- [ ] output length remains zero or otherwise well-defined on failure

---

## SR-005 — OpenPGP v4 / RFC 9580 Compatibility Position

**Severity:** Informational
**Status:** Documented

### Description

The implementation intentionally targets OpenPGP v4 for interoperability with
Keycard's existing hardware-backed secp256k1 capability and the GnuPG workflow
already demonstrated by the project.

This is not a claim that v4 is the preferred format for new general-purpose
OpenPGP implementations.

RFC 9580 / OpenPGP v6 is treated as a separate future standards track.

The current implementation:

- uses ECDSA with SHA-256 for certification signatures
- uses the v4 SHA-1 fingerprint construction because that construction is part
  of the v4 key format
- does not use SHA-1 as the certification signature hash
- does not claim RFC 9580 / v6 conformance
- does not claim standardized modern OpenPGP secp256k1 support

See `ROADMAP.md` for the compatibility disclosure and future v6 track.

---

## SR-006 — TLV / APDU Response Parsing Review

**Severity:** Low
**Confidence:** High
**Status:** Remediation submitted — [keycard-tech/keycard-shell#228](https://github.com/keycard-tech/keycard-shell/pull/228)
**Fix:** Keycard Shell commit `933fcf9` (`keycard: bound signature TLV response parsing`)
**Regression test:** firmware test-app cases for truncated direct and nested signature responses
**Verified by:** `shellos.elf` and `shellos-test.elf` build and sign successfully; physical execution of the new firmware self-test remains pending

### Description

The OpenPGP review identified a pre-existing weakness in the shared Keycard
signature-response parser.

`keycard_read_signature()` receives the logical APDU response length, but the
legacy TLV readers used by the function do not receive a source-buffer length.

In the direct-signature fast path, a response beginning with:

    80 41

declares a 65-byte value. If the logical response length is only two bytes, the
legacy fixed-primitive reader can still copy 65 bytes beginning after those two
bytes.

In the current APDU layout this has not been demonstrated to cross the allocated
APDU buffer. It can, however, read stale bytes beyond the logical response
boundary and interpret them as signature data.

The wrapped-signature path also called length-unaware tag and length readers
before all caller-side bounds checks had taken effect.

This behavior predates the OpenPGP changes and is shared with existing Keycard
signing paths. It is therefore an inherited Keycard Shell hardening issue, not a
vulnerability introduced by the OpenPGP implementation.

Secure Channel V2 authenticates and decrypts response plaintext before TLV
parsing, so an unauthenticated transport attacker cannot directly inject an
arbitrary malformed plaintext response without forging the channel
authentication. Malformed authenticated card responses remain relevant to
defensive parser hardening.

### Remediation

Keycard Shell PR [keycard-tech/keycard-shell#228](https://github.com/keycard-tech/keycard-shell/pull/228) adds bounds-aware variants of
the TLV tag, length, primitive, and fixed-primitive readers while preserving the
legacy API for existing callers.

`keycard_read_signature()` is migrated to those bounded helpers so that:

- truncated tags are rejected before reading missing tag bytes
- truncated long-form lengths are rejected before reading missing length bytes
- declared payload lengths are checked against the remaining logical response
  before `memcpy`
- the existing valid direct signature response format remains accepted

The remediation is implemented by commit `933fcf9`.

### Regression Tests

The firmware test app now includes cases for:

- [x] direct response `80 41` with a logical response length of two bytes
- [x] truncated direct long-form length
- [x] truncated wrapped long-form length
- [x] truncated nested tag
- [x] truncated nested length
- [x] valid direct 65-byte signature response remains accepted
- [x] rejected truncated responses do not modify the output signature buffer
- [ ] execute the new firmware self-test on development-capable Shell hardware

Both production and test firmware build and sign successfully with the
remediation applied.

Physical execution of the new firmware self-test remains pending.

---

# Investigated / Not Currently Findings

## Host-Controlled Derivation Path

Not observed in the production OpenPGP request flow.

The derivation path is absent from the request and is selected by Shell-owned
policy.

Current implementation:

    m/43'/60'/1581'/5261136'/0

Final child `x` semantics remain a policy question, but the host cannot select
the path.

## Signing Before User Approval

Not observed.

The approval flow completes before `keycard_cmd_sign()` is reached.

Cancel/reject returns before the signing operation.

Physical hardware testing is still required to confirm the full UI/event
behavior on the retail device.

## Ambiguous UID Display

The current identity confirmation path rejects UID bytes outside printable
ASCII before displaying and approving the UID.

This substantially reduces control-character and Unicode visual-confusion
issues for the current protocol.

## Generic `UR:BYTES` Signing Oracle

Not observed.

OpenPGP has a dedicated menu action and request handler.

The generic transaction QR dispatcher is not extended to treat arbitrary
`UR:BYTES` payloads as OpenPGP signing requests.

## Host-Supplied Digest or Signature

Not observed.

The Shell constructs the certification preimage and digest internally and the
Keycard produces the ECDSA signature.

## Unverified Certificate Output

Not observed.

The current orchestration assembles the certificate and performs binding and
cryptographic self-certification verification before returning the certificate.

---

# Remaining Cross-PR Audit Work

- [ ] finish PR #225 primitive review
- [ ] finish PR #226 CBOR/request parser review
- [ ] finish PR #227 orchestration review
- [x] complete TLV/APDU parser review
- [ ] review every error / cancellation path
- [ ] review output-length failure hygiene
- [ ] review stack/static-buffer bounds
- [ ] review request/output heap aliasing
- [ ] review malformed card responses
- [ ] confirm final derivation-policy semantics
- [ ] add regression tests for accepted findings
- [ ] run physical Shell E2E
- [ ] run final GnuPG validation

---

# Security Review Completion Criteria

The review is not considered complete until:

- [ ] no unresolved Critical finding remains
- [ ] no unresolved High finding remains
- [ ] every Medium finding is fixed, accepted, or explicitly deferred with rationale
- [ ] Low findings are fixed or explicitly documented
- [x] pending TLV/APDU review is completed
- [ ] cancellation/reject behavior is tested on physical hardware
- [ ] malformed request behavior is tested on physical hardware
- [ ] card/signing failure behavior is tested on physical hardware
- [ ] returned certificate passes final interoperability validation
- [ ] fingerprint shown by Shell matches the final certificate fingerprint

---

# Finding Update Rules

Do not delete a finding after fixing it.

Instead update:

    Status: Open

to something such as:

    Status: Fixed
    Fix: <commit / PR>
    Regression test: <test>
    Verified by: <build / test / physical E2E evidence>

If a finding is intentionally not changed:

    Status: Accepted
    Rationale: <reason>

If work is postponed:

    Status: Deferred
    Rationale: <reason>
    Follow-up: <issue / milestone>

This preserves the complete engineering review history.
