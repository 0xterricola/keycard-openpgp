# Keycard Shell — OpenPGP Project

## Overall Project

`██████████████████████████████░░  ~95%`

### Goal

    Host requests OpenPGP identity
            ↓
    Shell parses trusted request format
            ↓
    Shell derives Keycard-backed key
            ↓
    Shell derives OpenPGP fingerprint
            ↓
    Shell shows UID + Unix time + fingerprint
            ↓
    USER APPROVES
            ↓
    Keycard signs certification
            ↓
    Shell builds + verifies certificate
            ↓
    Certificate returned over UR:BYTES
            ↓
    GnuPG validates it

### Current State

- [x] complete CREATE_IDENTITY software vertical slice
- [x] runnable beta3 firmware build received from maintainer
- [x] physical Keycard Shell CREATE_IDENTITY E2E
- [x] multipart animated `UR:BYTES` response reconstructed
- [x] returned certificate accepted by GnuPG
- [x] valid self-signature demonstrated on physical hardware
- [x] current PR #227 maintainer review comments addressed
- [ ] PR #227 maintainer re-review
- [ ] remaining physical negative / cancellation-path validation
- [ ] upstream PR stack merge

The identity-creation happy path is now proven end-to-end on physical hardware.
The next functional layer, `SIGN_MESSAGE`, is tracked separately below.

---

# PR #225 — OpenPGP Primitives

**Branch:** `feature/openpgp-cert`

**PR:** https://github.com/keycard-tech/keycard-shell/pull/225

`████████████████████████████████  100% ✅ CODE COMPLETE`

## Commits

- `4073505` — `openpgp: add v4 packet parsing, digest and certification primitives`
- `1212e05` — `openpgp: build v4 public key body from secp256k1 point`

## Completed

- [x] OpenPGP packet parsing
- [x] v4 certification preimage
- [x] v4 signature fields
- [x] SHA-256 certification digest
- [x] v4 primary-key fingerprint
- [x] ECDSA r/s to OpenPGP MPI
- [x] signature packet builder
- [x] self-cert verification
- [x] secp256k1 public-key body
- [x] known-good GnuPG validation

## Status

- [x] branch pushed
- [x] PR #225 open
- [ ] review / merge pending

**Scope:** Keep this PR focused on reusable OpenPGP primitives.


---

# PR #226 — OpenPGP Request Protocol

**Branch:** `feature/openpgp-integration`

**PR:** https://github.com/keycard-tech/keycard-shell/pull/226

`████████████████████████████████  100% ✅ CODE COMPLETE`

## Commits

- `23b4eb6` — `openpgp: add versioned identity request protocol`
- `8b5a6d2` — `openpgp: carry creation time in identity requests`

## Request Format

    UR:BYTES
       ↓
    versioned CBOR

    {
      1: version,
      2: CREATE_IDENTITY,
      3: UID,
      4: creation_time
    }

## Completed

- [x] versioned protocol
- [x] CREATE_IDENTITY operation
- [x] UID transport
- [x] creation_time transport
- [x] malformed request rejection
- [x] wrong-version rejection
- [x] wrong-operation rejection
- [x] truncation rejection
- [x] trailing-data rejection
- [x] invalid UID rejection
- [x] zero creation_time rejection

## Security Boundary

Host may provide:

- [x] UID
- [x] creation_time

Host may NOT provide:

- [x] derivation path
- [x] public key
- [x] fingerprint
- [x] certification digest
- [x] signature

## Status

- [x] implementation complete
- [x] host-tested
- [x] firmware-tested
- [x] frozen checkpoint at `8b5a6d2`
- [x] branch pushed
- [x] PR #226 open
- [x] dependency on PR #225 documented
- [ ] rebase onto `master` after #225 lands
- [ ] reviewed / merged

## Dependency

**Depends on PR #225.**

PR #226 is stacked on top of the OpenPGP certification primitives in PR #225:

https://github.com/keycard-tech/keycard-shell/pull/225

Until #225 is merged, GitHub's comparison for #226 also includes the two prerequisite commits from #225.

While #225 is still unmerged, GitHub currently shows:

- 4 commits
- 7 files changed
- 1,178 additions

The protocol-specific delta remains:

- `23b4eb6` — `openpgp: add versioned identity request protocol`
- `8b5a6d2` — `openpgp: carry creation time in identity requests`
- 3 files changed
- 160 insertions

After #225 lands, `feature/openpgp-integration` can be rebased onto `master` so #226 shows only the protocol-specific changes.


---

# PR #227 — Trusted Identity Creation + Shell Integration

**Branch:** `feature/openpgp-orchestration`

**Remote:** `origin/feature/openpgp-orchestration`

**PR:** https://github.com/keycard-tech/keycard-shell/pull/227

**PR state:** open / maintainer re-review pending

**Current head:** `6f636d3`

`████████████████████████████████  100% ✅ IMPLEMENTATION + HARDWARE E2E COMPLETE`

## Completed Commits

- `a2bd3fe` — `openpgp: prepare primary key from Keycard path`
- `28fa692` — `openpgp: prepare UID certification digest`
- `c55ea7e` — `openpgp: confirm identity before certification`
- `8906ec6` — `openpgp: sign approved UID certification`
- `89c4262` — `openpgp: build UID certification packet`
- `da9852a` — `openpgp: use positive UID certification signature`
- `9613b81` — `openpgp: assemble and verify identity`
- `e50e282` — `openpgp: orchestrate identity creation`
- `374fb31` — `openpgp: add dedicated QR identity flow`
- `761ebc3` — `openpgp: define identity derivation path`
- `3c1f9f0` — `openpgp: add identity menu action`

## Maintainer Review Cleanup

- `03e485a` — `openpgp: move export path buffering to caller`
- `6601d32` — `style: align OpenPGP code with project conventions`
- `6445ceb` — `ui: simplify OpenPGP identity labels`
- `098a1a6` — `openpgp: group identity state`
- `6f636d3` — `style: trim OpenPGP comments`

### Review Status

- [x] export-buffer contract addressed
- [x] OpenPGP files aligned with project style
- [x] all `if` statements use braces
- [x] unnecessary line wrapping removed
- [x] redundant OpenPGP field prefixes removed
- [x] related identity state grouped internally
- [x] `core_openpgp_run()` is the only public core OpenPGP API
- [x] long narrating comments reduced to terse rationale notes
- [x] release build passes
- [x] `git diff --check` passes
- [x] all current maintainer review comments addressed
- [ ] maintainer re-review
- [ ] merge

## Completed

- [x] explicit trusted path passed into OpenPGP orchestration
- [x] OpenPGP path copied into padded `g_core.bip44_path` for Keycard export
- [x] Keycard public-key export
- [x] 65-byte secp256k1 public point
- [x] 79-byte OpenPGP v4 primary-key body
- [x] 20-byte OpenPGP fingerprint
- [x] UID certification preimage
- [x] positive UID certification signature type `0x13`
- [x] exact SHA-256 certification digest
- [x] display exact UID before signing
- [x] display derived fingerprint before signing
- [x] cancellable physical approval
- [x] Keycard ECDSA signing
- [x] signature decoding / normalization to `r || s`
- [x] OpenPGP signature packet construction
- [x] complete transferable certificate assembly
- [x] issuer fingerprint / key-ID binding checks
- [x] cryptographic self-cert verification before output
- [x] UID input snapshot for safe request/output aliasing
- [x] top-level identity orchestration helper
- [x] dedicated `UR:BYTES` QR request handler
- [x] versioned OpenPGP request parsing
- [x] verified certificate encoded as `UR:BYTES`
- [x] response QR display
- [x] generic `UR_ANY_TX` dispatch left unchanged
- [x] Shell-owned OpenPGP derivation policy wrapper
- [x] current implemented identity path `m/43'/60'/1581'/5261136'/0`
- [x] host/request cannot select or override that path
- [x] OpenPGP action under Extras
- [x] menu dispatch calls `core_openpgp_run()`
- [x] UI/task layer does not receive derivation-path parameters

## Current Pipeline

    OpenPGP action under Extras                 ✅
            ↓
    Shell-owned derivation policy            ✅
            ↓
    m/43'/60'/1581'/5261136'/0              ✅
            ↓
    dedicated UR:BYTES request              ✅
            ↓
    UID + creation_time                     ✅
            ↓
    Keycard EXPORT KEY                      ✅
            ↓
    secp256k1 public point                  ✅
            ↓
    OpenPGP primary-key body                ✅
            ↓
    OpenPGP fingerprint                     ✅
            ↓
    certification digest                    ✅
            ↓
    display UID + Unix time + fingerprint   ✅
            ↓
    physical user confirmation              ✅
            ↓
    Keycard signs digest                    ✅
            ↓
    decode r || s                           ✅
            ↓
    positive certification packet           ✅
            ↓
    assemble complete certificate           ✅
            ↓
    validate key/fingerprint binding        ✅
            ↓
    cryptographically self-verify           ✅
            ↓
    encode response as UR:BYTES             ✅
            ↓
    display response QR                     ✅

## Verification Checkpoint

- [x] release firmware builds successfully at current branch head
- [x] `git diff --check` passes
- [x] current branch pushed through `6f636d3`
- [x] upstream PR #227 open
- [x] maintainer review received
- [x] all current review comments addressed
- [x] PR body updated with physical beta3 E2E results
- [x] runnable beta3 firmware build received from maintainer
- [x] physical Shell CREATE_IDENTITY request scanned successfully
- [x] consolidated UID / Unix time / fingerprint review displayed
- [x] Keycard-backed certification completed
- [x] animated multipart response QR displayed without crash
- [x] host reconstructed response from 109 unique UR fragments
- [x] recovered certificate is 236 bytes
- [x] recovered packet sequence is `6 -> 13 -> 2`
- [x] GnuPG reports a valid self-signature
- [x] physical-test fingerprint:
  `69D20D8065CC8022444FB22FB0708911AC2833BC`

## Current Remaining Work

- [ ] maintainer re-review of PR #227
- [ ] maintainer confirmation of final child `x` semantics
- [ ] confirm whether any additional Keycard key-type policy is required
- [ ] verify remaining cancel / malformed-request / card-failure paths on hardware
- [ ] address any hardware-only failure-path issues discovered
- [ ] merge upstream PR stack

## PR Stack Note

PR #227 targets upstream `master` and remains stacked on the OpenPGP work from
PR #225 and PR #226.

The implementation and physical CREATE_IDENTITY happy path are complete.
The current stopping point is maintainer re-review of the cleaned-up PR.

No additional feature work should be added to PR #227 while that re-review is
pending. New OpenPGP functionality is being developed on a separate stacked
branch.

# Standards / Compatibility Disclosure

**Current interoperability target:** OpenPGP v4 + ECDSA/secp256k1 + GnuPG

The current implementation intentionally targets OpenPGP v4 because the
project began with Keycard's existing hardware-backed secp256k1 capability
and a concrete interoperability goal: derive the Keycard public key, construct
an OpenPGP identity around that same key material, have the Keycard sign the
certification digest, and produce a certificate accepted by GnuPG.

This is a deliberate compatibility target, not a claim that OpenPGP v4 is the
preferred format for new general-purpose OpenPGP implementations.

## Standards Position

- [x] current implementation explicitly targets OpenPGP v4
- [x] certification signatures use ECDSA with SHA-256
- [x] the v4 fingerprint uses SHA-1 because that is part of the v4 fingerprint
  construction; SHA-1 is not being used as the certification signature hash
- [x] RFC 9580 / OpenPGP v6 is recognized as the modern standards direction
  for newly generated OpenPGP keys
- [x] secp256k1 is not part of the standardized RFC 9580 OpenPGP ECC curve set
- [x] current implementation does not claim RFC 9580 / v6 conformance
- [x] current implementation does not claim standards endorsement of
  secp256k1 for modern OpenPGP
- [x] use of v4 is documented as an interoperability / existing-hardware
  decision rather than an unnoticed version choice

Using v4 for this Keycard interoperability work is not, by itself, evidence of
a cryptographic vulnerability or a violation of the OpenPGP model. It does,
however, carry a standards and lifecycle caveat: new general-purpose OpenPGP
implementations should evaluate the v6 path rather than assuming v4 is the
long-term target.

## Why v4 Exists Here

    existing Keycard hardware
            ↓
    secp256k1 key material
            ↓
    Keycard ECDSA / SHA-256 signing
            ↓
    OpenPGP v4 certificate construction
            ↓
    GnuPG interoperability
            ↓
    trusted Shell review + approval workflow

The purpose of the current PR stack is to make that specific hardware-backed
workflow explicit, reviewable, and testable.

## Future v6 Track

OpenPGP v6 should be investigated as a separate compatibility and standards
track rather than silently changing the semantics of the current v4 PR stack.

A future v6 investigation should determine:

- [ ] which RFC 9580-compatible signing algorithm can be supported by Keycard
- [ ] whether future standardized secp256k1 OpenPGP support becomes available
- [ ] v6 public-key packet construction
- [ ] SHA-256 / 32-byte v6 fingerprints
- [ ] v6 key-ID semantics
- [ ] salted v6 signature construction
- [ ] v6 self-certification verification
- [ ] interoperability with RFC 9580 implementations
- [ ] whether the existing OpenPGP derivation namespace can remain unchanged
- [ ] migration / coexistence policy between v4 and any future v6 identity

The current v4 work should remain usable as a documented interoperability
implementation even if a separate v6 path is added later.

## Security Review Status

The detailed defensive review of PRs #225 → #226 → #227 is tracked in
[`SECURITY_REVIEW.md`](./SECURITY_REVIEW.md).

That document records the threat model, findings, remediation status,
regression-test requirements, investigated non-findings, and remaining
cross-PR audit work.


Standards compatibility and security validation are separate questions.

Successful builds and GnuPG interoperability do not replace security review.
The current implementation is undergoing defensive review of the complete
PR #225 -> #226 -> #227 data path, including parsing, display/signing binding,
derivation-path ownership, memory safety, OpenPGP encoding, and failure paths.

Physical Shell happy-path E2E validation has completed using the maintainer's
beta3 firmware build. Negative, cancellation, malformed-request, and card-failure
paths remain part of defensive hardware validation.

---

# Derivation Policy

**Purpose:** Shell-owned OpenPGP derivation path

`█████████████████████████████░░░  ~90% 🟡 IMPLEMENTED / FINAL CHILD SEMANTICS PENDING`

## Current Implemented Policy

    m/43'/60'/1581'/5261136'/0

where:

    5261136 = 0x504750 = "PGP"

The final component is non-hardened. For the current implementation, child `0`
is treated as the initial OpenPGP identity/key index.

## Completed

- [x] host does not control derivation path
- [x] experimental Ethereum path rejected for production
- [x] SLIP-0013 investigated
- [x] SLIP-0017 investigated
- [x] ERC-1581 / Keycard namespace investigated
- [x] existing multisig EIP-1581 path identified
- [x] multisig already owns `m/43'/60'/1581'/0'/0`
- [x] OpenPGP does not silently reuse the multisig identity path
- [x] maintainer proposed dedicated OpenPGP namespace:
  `m/43'/60'/1581'/5261136'/x`
- [x] `5261136 = 0x504750 = "PGP"`
- [x] current implementation chooses `x = 0` for the initial identity
- [x] path constants encoded in Shell
- [x] no-argument `core_openpgp_run()` owns the production policy boundary
- [x] menu/action binds to the Shell-owned policy
- [x] path remains absent from the host request protocol
- [x] implementation isolated so the final child policy can change without redesigning orchestration
- [x] implementation commit: `761ebc3` — `openpgp: define identity derivation path`

## Remaining

- [ ] maintainer confirms exact meaning / policy for final child `x`
- [ ] confirm whether any additional Keycard key-type policy is required
- [ ] adjust the final child value/semantics if maintainer guidance differs from current `x = 0`
- [ ] finalize documentation wording after confirmation

## Current Position

The namespace question is resolved enough for implementation: OpenPGP has a
dedicated EIP-1581 namespace proposed by the maintainer and Shell now encodes it.

The only remaining policy uncertainty is the exact semantics of the final
non-hardened child `x` (and whether any additional Keycard key-type policy is
desired). The current code uses `x = 0` for the first identity and labels that
interpretation as pending final maintainer confirmation.

This is no longer blocking the software vertical slice, menu wiring, or PR #227.
If the maintainer requests a different child policy, the change is isolated to
the Shell-owned policy layer.


# Shell UI + QR Integration

`████████████████████████████████  100% ✅ CODE COMPLETE`

## Completed

- [x] existing `ui_qrscan()` supports explicit UR type
- [x] BYTES transport already available
- [x] existing QR output supports explicit BYTES
- [x] decided not to add generic BYTES to `UR_ANY_TX`
- [x] dedicated OpenPGP action architecture
- [x] `core_openpgp_qr_run()` implemented
- [x] `ui_qrscan(BYTES, ...)`
- [x] decode outer `UR:BYTES` CBOR byte string
- [x] parse versioned OpenPGP request
- [x] reject unsupported operation
- [x] show exact requested UID
- [x] show Unix creation time
- [x] show derived fingerprint
- [x] cancellable physical approval
- [x] execute approved identity workflow
- [x] construct and self-verify certificate
- [x] encode certificate as BYTES
- [x] display certificate response QR
- [x] request/output heap aliasing handled safely
- [x] generic transaction QR dispatch remains unchanged
- [x] OpenPGP action under Extras added
- [x] Extras menu dispatch wired to `core_openpgp_run()`
- [x] action bound to Shell-owned derivation policy
- [x] release build succeeds with the menu/action enabled
- [x] menu integration commit: `3c1f9f0` — `openpgp: add identity menu action`

## Physical Validation

- [x] OpenPGP action confirmed under Extras on physical Shell
- [x] physical `UR:BYTES` CREATE_IDENTITY request scanned
- [x] UID displayed
- [x] Unix creation time displayed
- [x] derived fingerprint displayed
- [x] physical approval completed
- [x] animated multipart response QR displayed
- [x] physical response reconstructed by host tooling
- [x] GnuPG validation passed
- [ ] cancel/reject path verified on physical hardware
- [ ] malformed-request failure behavior verified on physical hardware
- [ ] card/signing failure behavior verified on physical hardware

## Security Rule

Generic QR scanning must not accidentally turn arbitrary `UR:BYTES`
payloads into OpenPGP signing requests.

OpenPGP has a dedicated purpose-specific menu action and explicitly requests
`BYTES`; the existing generic transaction QR dispatcher remains unchanged.

The UI/task dispatcher invokes `core_openpgp_run()` without accepting a path,
so the host and menu layer cannot select the Keycard derivation path.


# Host E2E Tooling

**Repository:** `0xterricola/keycard-openpgp`

**Directory:** `experiments/shell-port/host-e2e`

**Initial host tooling PR:** #23 — merged

**Multipart physical-response PR:** #36 — merged

https://github.com/0xterricola/keycard-openpgp/pull/36

**Development branch used for multipart work:** `feature/host-e2e-multipart-ur`

**Scope:** Host-side test tooling in the project tracker/research repository.
This is not part of the `keycard-tech/keycard-shell` firmware PR stack.

`████████████████████████████████  100% ✅ HOST TOOLING + PHYSICAL E2E COMPLETE`

## Request Generator

- [x] isolated under `experiments/shell-port/host-e2e`
- [x] CREATE_IDENTITY request format
- [x] version = `1`
- [x] operation = `CREATE_IDENTITY`
- [x] UID encoded as CBOR byte string
- [x] `creation_time` encoded as uint32
- [x] host cannot provide derivation path
- [x] host cannot provide public key
- [x] host cannot provide fingerprint
- [x] host cannot provide certification digest
- [x] host cannot provide signature
- [x] inner CBOR wrapped as `UR:BYTES`
- [x] QR image generated
- [x] generated UR self-decodes byte-for-byte
- [x] QR scan reproduces the exact UR payload

## Response Decoder

- [x] accepts `UR:BYTES`
- [x] supports multipart animated UR responses
- [x] consumes fountain fragments from physical Shell capture
- [x] reconstructs complete OpenPGP certificate
- [x] requires packet sequence `6 -> 13 -> 2`
- [x] writes recovered `.pgp` certificate
- [x] known-good certificate round-trips byte-for-byte
- [x] physical beta3 response reconstructed successfully
- [x] 109 unique UR fragments consumed in physical validation
- [x] recovered physical certificate size: 236 bytes

## Physical Test Result

    CREATE_IDENTITY request
            ↓
    UR:BYTES
            ↓
    physical Keycard Shell beta3
            ↓
    Keycard-backed certification
            ↓
    animated multipart UR:BYTES
            ↓
    host video capture / fragment extraction
            ↓
    multipart decoder
            ↓
    OpenPGP certificate
            ↓
    packet sequence 6 -> 13 -> 2
            ↓
    GnuPG
            ↓
    valid self-signature ✅

Physical-test fingerprint:

`69D20D8065CC8022444FB22FB0708911AC2833BC`

## Platform Documentation

- [x] macOS physical E2E validated
- [x] Linux instructions documented
- [x] Windows instructions documented
- [ ] Linux physical E2E independently validated
- [ ] Windows physical E2E independently validated

The host side is no longer a blocker for CREATE_IDENTITY.

---

# Final Device + GnuPG Validation

`██████████████████████████████░░  ~95% ✅ HAPPY-PATH E2E COMPLETE`

## Physical Shell beta3 E2E

- [x] received runnable maintainer firmware build
- [x] OpenPGP action available under Extras
- [x] generated physical-test CREATE_IDENTITY request
- [x] scanned request with physical Shell
- [x] Shell derived physical Keycard public key
- [x] Shell displayed requested UID
- [x] Shell displayed Unix creation time
- [x] Shell displayed derived fingerprint
- [x] user physically approved
- [x] Keycard signed certification digest
- [x] Shell constructed certificate
- [x] Shell cryptographically self-verified certificate
- [x] Shell displayed animated multipart `UR:BYTES` response
- [x] host captured physical Shell response
- [x] 109 unique UR fragments recovered
- [x] multipart response reconstructed
- [x] recovered certificate size: 236 bytes
- [x] packet sequence verified as `6 -> 13 -> 2`
- [x] GnuPG accepted certificate
- [x] GnuPG reported valid self-signature

## Physical Test Artifact

UID:

`Keycard Test <keycard@example.com>`

Fingerprint:

`69D20D8065CC8022444FB22FB0708911AC2833BC`

Proof:

    Recovered certificate: 236 bytes
    Packet sequence:       6 -> 13 -> 2
    RESPONSE STRUCTURE CHECK: PASS
    gpg: 1 good signature

## Remaining Defensive Validation

- [ ] cancel/reject path on physical hardware
- [ ] malformed-request failure behavior
- [ ] card/signing failure behavior
- [ ] address any hardware-only failure-path issues discovered

## CREATE_IDENTITY Success Condition

        CREATE_IDENTITY REQUEST
             |
             v
        EXTRAS -> OPENPGP
             |
             v
        SHELL TRUSTED REVIEW
             |
             | UID
             | Unix creation time
             | fingerprint
             v
        USER APPROVES
             |
             v
        KEYCARD SIGNS
             |
             v
     SHELL SELF-VERIFIES
             |
             v
      MULTIPART UR:BYTES
             |
             v
          HOST
             |
             v
          GNUPG
             |
             v
      VALID SELF-SIGNATURE ✅

The physical happy-path success condition has been met.

# Phase 2 — OpenPGP SIGN_MESSAGE

**Shell branch:** `feature/openpgp-sign-message`

**Base:** `feature/openpgp-orchestration`

**Status:** protocol alignment / implementation pending

`███░░░░░░░░░░░░░░░░░░░░░░░░░░░  DESIGN`

## Goal

    application statement
            ↓
    versioned SIGN_MESSAGE request
            ↓
    UR:BYTES
            ↓
    Shell reconstructs existing OpenPGP identity
            ↓
    Shell displays exact statement + fingerprint
            ↓
    USER APPROVES
            ↓
    Keycard signs OpenPGP message digest
            ↓
    Shell builds detached OpenPGP signature
            ↓
    Shell verifies signature
            ↓
    Signature packet returned as UR:BYTES

## Proposed Request

    {
      1: 1,  // protocol version
      2: 2,  // SIGN_MESSAGE
      3: <message bytes>,
      4: <key_creation_time>,
      5: <signature_creation_time>
    }

Types:

    1 -> uint
    2 -> uint
    3 -> bstr
    4 -> uint32
    5 -> uint32

## Timestamp Semantics

`key_creation_time`

- original creation time of the existing OpenPGP v4 primary key
- remains stable for that identity
- is part of the v4 public-key packet
- therefore participates in reconstruction of the same v4 fingerprint

`signature_creation_time`

- creation time of the new message signature
- changes for each signed statement
- is included in the hashed OpenPGP signature metadata

The two timestamps describe two different OpenPGP objects.

## Proposed Signing Semantics

- [ ] OpenPGP v4 canonical-text document signature type `0x01`
- [ ] exact reviewed statement bytes
- [ ] reviewable text only for the initial operation
- [ ] Shell-owned derivation path
- [ ] host cannot provide derivation path
- [ ] host cannot provide fingerprint
- [ ] host cannot provide digest
- [ ] host cannot provide signature
- [ ] standard detached OpenPGP Signature packet returned as `UR:BYTES`

The request/response shape remains proposed until application-side compatibility
is confirmed.

## Existing Thurin Proof

The earlier hardware-attestation experiment already demonstrated:

    canonical Thurin ownership statement
            ↓
    trusted on-device review
            ↓
    hardware-backed OpenPGP canonical-text signature
            ↓
    standard OpenPGP Signature packet
            ↓
    GnuPG verification
            ↓
    @thurinlabs/identity-kit verification

The prototype statement shape was:

    I control the Ethereum address: 0x<40 hexadecimal characters>

The prototype used OpenPGP signature type:

    0x01 — canonical text document

## Current Thurin Coordination

- [x] CREATE_IDENTITY architecture shared with Ben
- [x] working Shell host `UR:BYTES` implementation shared
- [x] proposed SIGN_MESSAGE request shape shared
- [x] proposed detached-signature response discussed
- [x] direct Thurin -> Shell QR architecture discussed
- [ ] Ben checks compatibility with `identity-kit` and current PGP flow
- [ ] freeze request/response schema
- [ ] implement Shell SIGN_MESSAGE
- [ ] extend host E2E tooling
- [ ] physical Shell message-signing E2E
- [ ] verify detached signature with GnuPG
- [ ] verify result with `@thurinlabs/identity-kit`
- [ ] first hardware-backed Thurin Sepolia claim

## Current Stopping Point

Do not modify PR #227 for SIGN_MESSAGE.

PR #227 remains stable for maintainer re-review.

Continue SIGN_MESSAGE on:

`feature/openpgp-sign-message`

Implementation begins after the request/response semantics are aligned with the
existing Thurin / identity-kit flow.

---

# Branch / PR Stack

    upstream/master
       |
       v
    feature/openpgp-cert
       |
       +-- PR #225 OPEN
       +-- REVIEW / MERGE PENDING
       |
       v
    feature/openpgp-integration
       |
       +-- PR #226 OPEN
       +-- DEPENDS ON PR #225
       +-- REVIEW / MERGE PENDING
       |
       v
    feature/openpgp-orchestration
       |
       +-- PR #227 OPEN
       +-- CURRENT HEAD: 6f636d3
       +-- PHYSICAL CREATE_IDENTITY E2E PASS
       +-- CURRENT REVIEW COMMENTS ADDRESSED
       +-- MAINTAINER RE-REVIEW PENDING
       |
       +-------------------------------+
       |
       v
    feature/openpgp-sign-message
       |
       +-- STACKED FROM PR #227 WORK
       +-- BRANCH CREATED
       +-- PROTOCOL ALIGNMENT PENDING
       +-- NO IMPLEMENTATION YET

PR #227 should remain stable while maintainer re-review is pending.

After the prerequisite PRs land, the stack can be rebased/updated so each PR
contains only its intended delta.

# Current Exact Position

    OpenPGP primitives                         ✅ PR #225
            ↓
    CREATE_IDENTITY request protocol           ✅ PR #226
            ↓
    trusted Shell orchestration                ✅
            ↓
    Shell-owned OpenPGP derivation policy      ✅
            ↓
    Extras -> OpenPGP action                   ✅
            ↓
    Keycard public-key export                  ✅
            ↓
    OpenPGP primary-key body                   ✅
            ↓
    fingerprint derivation                     ✅
            ↓
    UID certification digest                   ✅
            ↓
    trusted UID / time / fingerprint review    ✅
            ↓
    physical user approval                     ✅
            ↓
    Keycard certification signature            ✅
            ↓
    positive UID certification packet          ✅
            ↓
    certificate assembly                       ✅
            ↓
    fingerprint / issuer binding               ✅
            ↓
    cryptographic self-verification            ✅
            ↓
    dedicated UR:BYTES request                 ✅
            ↓
    animated multipart UR:BYTES response       ✅
            ↓
    physical beta3 Shell E2E                   ✅
            ↓
    host multipart reconstruction              ✅
            ↓
    GnuPG valid self-signature                 ✅
            ↓
    maintainer review feedback                 ✅
            ↓
    review cleanup                             ✅
            ↓
    PR #227 body / verification updated        ✅
            ↓
    +-----------------------------------------+
    | PR #227 MAINTAINER RE-REVIEW           |
    |                                         |
    |      ← IDENTITY FLOW IS HERE           |
    +-----------------------------------------+

Parallel next layer:

    feature/openpgp-sign-message               ✅ CREATED
            ↓
    Thurin / identity-kit protocol alignment   ⏳
            ↓
    freeze SIGN_MESSAGE protocol               ⬜
            ↓
    Shell implementation                       ⬜
            ↓
    physical message-signing E2E               ⬜
            ↓
    first hardware-backed Thurin claim         ⬜

Parallel policy item still pending:

    maintainer confirms exact child x semantics / key-type policy ⏳

# Project Snapshot

    PR #225 — OpenPGP primitives
    ████████████████████████████████ 100% ✅
    CODE COMPLETE / OPEN / REVIEW-MERGE PENDING

    PR #226 — OpenPGP request protocol
    ████████████████████████████████ 100% ✅
    CODE COMPLETE / OPEN / DEPENDS ON #225

    PR #227 — identity creation + Shell integration
    ████████████████████████████████ 100% ✅
    IMPLEMENTATION COMPLETE / PHYSICAL E2E PASS / RE-REVIEW PENDING

    Derivation policy
    █████████████████████████████░░░  90% 🟡
    NAMESPACE + x=0 IMPLEMENTED / FINAL x SEMANTICS PENDING

    Shell UI + QR integration
    ████████████████████████████████ 100% ✅
    EXTRAS ACTION + PHYSICAL QR HAPPY PATH COMPLETE

    Host E2E tooling
    ████████████████████████████████ 100% ✅
    MULTIPART PHYSICAL E2E COMPLETE / PR #36 MERGED

    Device + GnuPG E2E
    ██████████████████████████████░░  95% ✅
    HAPPY PATH COMPLETE / DEFENSIVE FAILURE PATHS REMAIN

    SIGN_MESSAGE
    ███░░░░░░░░░░░░░░░░░░░░░░░░░░░  DESIGN 🟡
    STACKED BRANCH CREATED / THURIN PROTOCOL ALIGNMENT PENDING

    CREATE_IDENTITY overall
    ██████████████████████████████░░  95% ✅
    PHYSICAL VERTICAL SLICE COMPLETE / UPSTREAM RE-REVIEW + MERGE REMAIN

## Next Checkbox

### CREATE_IDENTITY / PR #227

- [x] host CREATE_IDENTITY request / response QR tooling
- [x] maintainer proposes dedicated OpenPGP namespace
  `m/43'/60'/1581'/5261136'/x`
- [x] implement current `x = 0` Shell-owned policy
- [x] bind policy to `core_openpgp_run()`
- [x] add OpenPGP action under Extras
- [x] release-build the complete software flow
- [x] open upstream PR #227
- [x] receive runnable beta3 firmware build
- [x] run physical Shell CREATE_IDENTITY E2E
- [x] reconstruct animated multipart response
- [x] validate returned certificate with GnuPG
- [x] receive maintainer review
- [x] address all current maintainer review comments
- [x] update PR body with physical E2E result
- [ ] maintainer re-review
- [ ] maintainer confirms final child `x` semantics / any additional key-type policy
- [ ] verify cancel / malformed-request / card-failure behavior on hardware
- [ ] address any hardware-only failure-path issues discovered
- [ ] review / merge PR #225, then #226, then #227

### SIGN_MESSAGE

- [x] create `feature/openpgp-sign-message`
- [x] inspect existing generic OpenPGP v4 signature primitives
- [x] inspect previous Thurin hardware-attestation experiment
- [x] propose version-1 SIGN_MESSAGE CBOR shape
- [x] explain two-timestamp v4 identity/signature model
- [x] share Shell PR + host UR tooling with Ben
- [x] discuss direct Thurin -> Shell `UR:BYTES` QR flow
- [ ] receive Ben's identity-kit / PGP compatibility feedback
- [ ] freeze request/response schema
- [ ] implement protocol parsing
- [ ] implement trusted message review
- [ ] implement Keycard-backed canonical-text signing
- [ ] implement detached-signature self-verification
- [ ] return Signature packet through `UR:BYTES`
- [ ] extend host E2E tooling
- [ ] test on physical Shell
- [ ] verify with GnuPG
- [ ] verify with `@thurinlabs/identity-kit`
- [ ] execute first hardware-backed Thurin Sepolia claim
