# Keycard Shell — OpenPGP Identity Project

## Overall Project

`█████████████████████████████░░░  ~90%`

### Goal

    Host requests OpenPGP identity
            ↓
    Shell parses trusted request format
            ↓
    Shell derives Keycard-backed key
            ↓
    Shell derives OpenPGP fingerprint
            ↓
    Shell shows UID + fingerprint
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

**PR state:** ready for review

**Current head:** `3c1f9f0`

`████████████████████████████████  100% ✅ CODE COMPLETE`

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

## Completed

- [x] explicit trusted path passed into OpenPGP orchestration
- [x] no mutation of wallet-oriented `g_core.bip44_path`
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
- [x] OpenPGP main-menu entry
- [x] menu dispatch calls `core_openpgp_run()`
- [x] UI/task layer does not receive derivation-path parameters

## Current Pipeline

    OpenPGP main-menu action                 ✅
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
    display UID + fingerprint               ✅
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

- [x] `git diff --check` passed for derivation-policy changes
- [x] release firmware built successfully after derivation-policy changes
- [x] `git diff --check` passed for menu integration
- [x] release firmware built successfully at `3c1f9f0`
- [x] previous clean full release build reached `385/385`
- [x] core task stack confirmed as 2048 × 32-bit = 8192 bytes
- [x] `core_openpgp_create_identity_at_path()` frame: 680 bytes
- [x] `core_openpgp_qr_run()` frame: 72 bytes
- [x] branch pushed through `3c1f9f0`
- [x] upstream PR #227 opened
- [x] PR #227 marked ready for review
- [x] PR #227 documents dependencies on #225 and #226

## Current Remaining Work

- [ ] maintainer confirmation of final child `x` semantics
- [ ] confirm whether any additional Keycard key-type policy is required
- [ ] receive a runnable firmware build containing PR #227
- [ ] physical Shell CREATE_IDENTITY E2E
- [ ] final GnuPG validation
- [ ] fix any hardware-only UX / failure behavior discovered during E2E

## PR Stack Note

PR #227 targets upstream `master` and is stacked on #225 and #226.

Because the prerequisite PRs are not yet merged, GitHub currently shows the full stacked history in #227 (15 commits). After the prerequisite PRs land and the stack is rebased/updated, the review diff can collapse to the orchestration / path-policy / menu-specific delta.



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

Physical Shell E2E validation remains required before the implementation is
considered fully validated on production hardware.

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
- [x] show derived fingerprint
- [x] cancellable physical approval
- [x] execute approved identity workflow
- [x] construct and self-verify certificate
- [x] encode certificate as BYTES
- [x] display certificate response QR
- [x] request/output heap aliasing handled safely
- [x] generic transaction QR dispatch remains unchanged
- [x] OpenPGP main-menu entry added
- [x] main-menu dispatch wired to `core_openpgp_run()`
- [x] action bound to Shell-owned derivation policy
- [x] release build succeeds with the menu/action enabled
- [x] menu integration commit: `3c1f9f0` — `openpgp: add identity menu action`

## Remaining Validation

- [ ] confirm user-facing failure behavior on physical hardware
- [ ] physical QR request/response test
- [ ] confirm menu/action behavior on a runnable firmware build

## Security Rule

Generic QR scanning must not accidentally turn arbitrary `UR:BYTES`
payloads into OpenPGP signing requests.

OpenPGP has a dedicated purpose-specific menu action and explicitly requests
`BYTES`; the existing generic transaction QR dispatcher remains unchanged.

The UI/task dispatcher invokes `core_openpgp_run()` without accepting a path,
so the host and menu layer cannot select the Keycard derivation path.


# Host E2E Tooling

**Repository:** `0xterricola/keycard-openpgp`

**Branch:** `feature/openpgp-host-e2e`

**Commit:** `194bd8f` — `test: add OpenPGP Shell host E2E tooling`

**PR:** #23 — merged into `0xterricola/keycard-openpgp` `main`

**Scope:** Host-side test tooling in the project tracker/research repository. This is not part of the `keycard-tech/keycard-shell` firmware PR stack.

`████████████████████████████████  100% ✅ HOST TOOLING COMPLETE`

## Request Generator

- [x] isolated under `experiments/shell-port/host-e2e`
- [x] frozen CREATE_IDENTITY request format
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

- [x] accepts `UR:BYTES` text
- [x] accepts `UR:BYTES` file
- [x] accepts QR image
- [x] recovers complete OpenPGP certificate
- [x] requires packet sequence `6 -> 13 -> 2`
- [x] writes recovered `.pgp` certificate
- [x] known-good certificate round-trips byte-for-byte
- [x] response QR round-trips byte-for-byte

## Proven Test Vectors

Request:

    CBOR
      ↓
    UR:BYTES
      ↓
    QR
      ↓
    zbar scan
      ↓
    identical UR payload ✅

Response:

    known-good OpenPGP certificate
      ↓
    UR:BYTES
      ↓
    QR
      ↓
    host decoder
      ↓
    identical certificate bytes ✅

## Remaining

The host tooling is ready for the physical Shell test.

It intentionally does not define or transmit the OpenPGP derivation path.
That is now implemented as Shell-owned policy at
`m/43'/60'/1581'/5261136'/0`, while final maintainer confirmation of the child
`x` semantics remains pending.

The host side is not the current blocker. Physical E2E is waiting on a runnable
firmware build containing PR #227.


---

# Final Device + GnuPG Validation

`███████████████░░░░░░░░░░░░░░░░░  ~45% ⬜ BLOCKED ON RUNNABLE FIRMWARE BUILD`

## Already Proven

- [x] original Keycard applet experiment
- [x] Keycard-derived secp256k1 key
- [x] Keycard signs OpenPGP certification digest
- [x] assembled certificate accepted by GnuPG
- [x] valid self-signature demonstrated
- [x] primitive host harness
- [x] request-protocol host harness
- [x] complete Shell-side identity construction path
- [x] Shell-side certificate self-verification path
- [x] dedicated Shell `UR:BYTES` identity handler builds
- [x] host CREATE_IDENTITY request generator implemented and QR round-trip verified
- [x] host response decoder implemented and QR round-trip verified
- [x] Shell-owned OpenPGP path policy implemented
- [x] current policy uses `m/43'/60'/1581'/5261136'/0`
- [x] OpenPGP main-menu action implemented
- [x] menu action bound to `core_openpgp_run()`
- [x] release firmware builds successfully with the complete software flow
- [x] previous clean full release build reaches `385/385`
- [x] upstream PR #227 opened and marked ready for review

## Current Hardware Blocker

A physical Shell E2E cannot be run from the locally built development-signed
firmware on the available retail device.

The physical test is waiting for the firmware developer to provide a runnable
build containing PR #227 / `feature/openpgp-orchestration`.

This build dependency is already understood; no additional firmware-flashing
work is part of the OpenPGP implementation task right now.

## Final Shell E2E Still Required

- [ ] maintainer confirms final child `x` semantics / any additional key-type policy
- [ ] receive runnable firmware build containing PR #227
- [ ] confirm OpenPGP main-menu entry on physical Shell
- [ ] generate final physical-test CREATE_IDENTITY request
- [ ] scan request with physical Shell
- [ ] Shell derives physical Keycard public key
- [ ] Shell displays requested UID
- [ ] Shell displays derived fingerprint
- [ ] user physically approves
- [ ] Keycard signs certification digest
- [ ] Shell constructs certificate
- [ ] Shell verifies certificate before returning it
- [ ] Shell displays certificate as response QR
- [ ] host scans physical Shell response
- [ ] save/import certificate
- [ ] GnuPG accepts certificate
- [ ] GnuPG reports valid self-signature
- [ ] GnuPG fingerprint matches Shell fingerprint
- [ ] cancel/reject path verified
- [ ] malformed-request failures verified
- [ ] card/signing failure behavior verified

## Final Success Condition

        RUNNABLE FIRMWARE BUILD
             |
             v
        OPENPGP MENU ACTION
             |
             v
        CREATE_IDENTITY QR
             |
             v
        SHELL DISPLAY
             |
             | UID + fingerprint
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
       UR:BYTES OUTPUT
             |
             v
          GNUPG
             |
             v
     VALID SELF-SIGNATURE
             |
             v
    GnuPG fingerprint == Shell fingerprint ✅


# Branch / PR Stack

    upstream/master
       |
       v
    feature/openpgp-cert
    4073505
    1212e05
       |
       +-- PR #225 OPEN ✅
       +-- REVIEW / MERGE PENDING
       |
       v
    feature/openpgp-integration
    23b4eb6
    8b5a6d2
       |
       +-- PR #226 OPEN ✅
       +-- DEPENDS ON PR #225
       +-- REVIEW / MERGE PENDING
       |
       v
    feature/openpgp-orchestration
    a2bd3fe
    28fa692
    c55ea7e
    8906ec6
    89c4262
    da9852a
    9613b81
    e50e282
    374fb31
    761ebc3
    3c1f9f0
       |
       +-- PR #227 OPEN / READY FOR REVIEW ✅
       +-- DEPENDS ON PR #225 + PR #226
       +-- CURRENT HEAD: 3c1f9f0
       +-- CURRENTLY SHOWS FULL 15-COMMIT STACK
       |
       v
    runnable firmware build from PR #227
       |
       +-- WAITING ON FIRMWARE DEVELOPER ⏳
       |
       v
    physical Shell E2E
       |
       v
    final GnuPG validation


# Current Exact Position

    OpenPGP primitives                    ✅ PR #225
            ↓
    Request protocol                      ✅ PR #226
            ↓
    UID + creation_time                   ✅
            ↓
    trusted path interface                ✅
            ↓
    Keycard public-key export             ✅
            ↓
    OpenPGP primary-key body              ✅
            ↓
    Fingerprint derivation                ✅
            ↓
    Certification digest                  ✅
            ↓
    Display UID + fingerprint             ✅
            ↓
    User approval                         ✅
            ↓
    Keycard certification signature       ✅
            ↓
    Positive UID certification packet     ✅
            ↓
    Certificate assembly                  ✅
            ↓
    Fingerprint / issuer binding           ✅
            ↓
    Cryptographic self-verification       ✅
            ↓
    Dedicated UR:BYTES request            ✅
            ↓
    UR:BYTES certificate response         ✅
            ↓
    Host request / response tooling       ✅
            ↓
    OpenPGP namespace                     ✅
    m/43'/60'/1581'/5261136'/x
            ↓
    Current child policy                  ✅ PROVISIONAL
    x = 0
            ↓
    Shell path-policy wrapper             ✅
            ↓
    OpenPGP menu/action                   ✅
            ↓
    PR #227                               ✅ READY FOR REVIEW
            ↓
    +------------------------------------+
    | RUNNABLE FIRMWARE BUILD            |
    | from PR #227                       |
    |      ← WE ARE HERE                 |
    +------------------------------------+
            ↓
    Physical Shell E2E                   ⬜
            ↓
    Final GnuPG validation               ⬜

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
    CODE COMPLETE / READY FOR REVIEW / DEPENDS ON #225 + #226

    Derivation policy
    █████████████████████████████░░░  90% 🟡
    NAMESPACE + x=0 IMPLEMENTED / FINAL x SEMANTICS PENDING

    Shell UI + QR integration
    ████████████████████████████████ 100% ✅
    MENU + QR FLOW COMPLETE / PHYSICAL VALIDATION PENDING

    Host E2E tooling
    ████████████████████████████████ 100% ✅
    MERGED INTO 0xterricola/keycard-openpgp MAIN / NOT SHELL FIRMWARE

    Device + GnuPG E2E
    ███████████████░░░░░░░░░░░░░░░░░  45% ⬜
    WAITING ON RUNNABLE PR #227 FIRMWARE BUILD

    Overall project
    █████████████████████████████░░░  90% 🟡
    SOFTWARE VERTICAL SLICE COMPLETE / HARDWARE E2E REMAINS


## Next Checkbox

- [x] host CREATE_IDENTITY request / response QR tooling
- [x] maintainer proposes dedicated OpenPGP namespace
  `m/43'/60'/1581'/5261136'/x`
- [x] implement current `x = 0` Shell-owned policy
- [x] bind policy to `core_openpgp_run()`
- [x] add OpenPGP main-menu action
- [x] release-build the complete software flow
- [x] open upstream PR #227
- [x] mark PR #227 ready for review
- [ ] maintainer confirms final child `x` semantics / any additional key-type policy
- [ ] receive runnable firmware build containing PR #227
- [ ] run first physical Shell CREATE_IDENTITY E2E
- [ ] validate returned certificate with GnuPG
- [ ] verify cancel / malformed-request / card-failure behavior on hardware
- [ ] address any hardware-only issues discovered during E2E
- [ ] review / merge PR #225, then #226, then #227
