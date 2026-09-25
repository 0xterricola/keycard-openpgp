# Keycard Shell — OpenPGP Identity Project

## Overall Project

`██████████████████████████░░░░░░  ~80%`

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
- [x] Draft PR #225 open
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
- [x] Draft PR #226 open
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

# OpenPGP Identity Orchestration

**Branch:** `feature/openpgp-orchestration`

**Remote:** `origin/feature/openpgp-orchestration`

**Standalone PR:** not opened; branch is kept as a pushed checkpoint for now

`████████████████████████████████  100% ✅ CORE FLOW COMPLETE`

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

## Current Pipeline

    Shell-selected trusted path              ✅
            ↓
    Keycard EXPORT KEY                      ✅
            ↓
    secp256k1 public point                  ✅
            ↓
    OpenPGP primary-key body                ✅
            ↓
    OpenPGP fingerprint                     ✅
            ↓
    UID + creation_time                     ✅
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

- [x] `git diff --check` clean
- [x] clean release build passes `385/385`
- [x] core task stack confirmed as 2048 × 32-bit = 8192 bytes
- [x] `core_openpgp_create_identity_at_path()` frame: 680 bytes
- [x] `core_openpgp_qr_run()` frame: 72 bytes
- [x] branch pushed and tracking `origin/feature/openpgp-orchestration`

## Remaining Outside Core Orchestration

- [ ] choose final Shell-owned production derivation path
- [ ] add OpenPGP menu/action entry
- [ ] bind menu action to approved path policy
- [ ] physical Shell end-to-end test
- [ ] final GnuPG validation

## PR Strategy

No standalone orchestration PR is being opened at this checkpoint.

The branch is pushed for backup, comparison, and review while the production derivation-path policy is resolved. After that decision, the remaining integration can be completed and folded into the existing OpenPGP PR stack.


# Derivation Policy

**Purpose:** Final Shell-owned OpenPGP derivation path

`████████████░░░░░░░░░░░░░░░░░░░░  ~40% ⏳ BLOCKED ON GUIDANCE`

## Completed

- [x] host does not control derivation path
- [x] experimental Ethereum path rejected for production
- [x] SLIP-0013 investigated
- [x] SLIP-0017 investigated
- [x] ERC-1581 / Keycard namespace investigated
- [x] path decision isolated from orchestration plumbing
- [x] existing multisig EIP-1581 path identified
- [x] multisig already owns `m/43'/60'/1581'/0'/0`
- [x] OpenPGP will not silently reuse the multisig identity path
- [x] maintainer question sent
- [x] orchestration remains path-agnostic until policy is decided

## Remaining

- [ ] exact OpenPGP production namespace / key type / path
- [ ] maintainer approval of reserved path
- [ ] define final path constants/helper
- [ ] wire final policy into OpenPGP menu/action
- [ ] document rationale

## Expected Commit

    openpgp: define identity derivation path

## Current Blocker

The Shell-side cryptographic and QR plumbing is complete.

The remaining policy question is which distinct Keycard / EIP-1581 path should be reserved for OpenPGP. Multisig already uses `m/43'/60'/1581'/0'/0`, so OpenPGP must not accidentally collide with that identity key.

This blocks final menu wiring and physical E2E testing, but it does not require redesigning the completed orchestration APIs.


# Shell UI + QR Integration

`███████████████████████████░░░░░  ~85% 🟡`

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

## Remaining

- [ ] add OpenPGP menu/action entry
- [ ] bind action to final Shell-owned derivation path
- [ ] add/confirm user-facing failure behavior during physical testing
- [ ] physical QR request/response test

## Security Rule

Generic QR scanning must not accidentally turn arbitrary `UR:BYTES`
payloads into OpenPGP signing requests.

OpenPGP has a dedicated purpose-specific action and explicitly requests
`BYTES`; the existing generic transaction QR dispatcher remains unchanged.


# Host E2E Tooling

**Branch:** `feature/openpgp-host-e2e`

**Commit:** `194bd8f` — `test: add OpenPGP Shell host E2E tooling`

**PR:** open in `keycard-openpgp`

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
That remains trusted Shell policy and is still blocked on maintainer guidance.


---

# Final Device + GnuPG Validation

`███████████░░░░░░░░░░░░░░░░░░░░░  ~35% ⬜`

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
- [x] repeated production firmware builds
- [x] repeated test firmware builds
- [x] clean full release build reaches `385/385`

## Final Shell E2E Still Required

- [ ] finalize production derivation path
- [ ] expose OpenPGP action in Shell menu
- [x] host CREATE_IDENTITY request generator implemented and QR round-trip verified
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
- [x] host response decoder implemented and QR round-trip verified
- [ ] host scans physical Shell response
- [ ] save/import certificate
- [ ] GnuPG accepts certificate
- [ ] GnuPG reports valid self-signature
- [ ] GnuPG fingerprint matches Shell fingerprint
- [ ] cancel/reject path verified
- [ ] malformed-request failures verified
- [ ] card/signing failure behavior verified

## Final Success Condition

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
    825ba4c
       |
       v
    feature/openpgp-cert
    4073505
    1212e05
       |
       +-- PR #225 OPEN ✅
       |
       v
    feature/openpgp-integration
    23b4eb6
    8b5a6d2
       |
       +-- PR #226 DRAFT OPEN ✅
       |   DEPENDS ON PR #225
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
       |
       +-- PUSHED TO origin ✅
       +-- NO STANDALONE PR RIGHT NOW
       |
       v
    derivation path policy
       |
       +-- WAITING ON MAINTAINER ⏳
       |
       v
    menu wiring + physical E2E


# Current Exact Position

    Request protocol                     ✅
            ↓
    UID + creation_time                  ✅
            ↓
    trusted path interface               ✅
            ↓
    Keycard public-key export            ✅
            ↓
    OpenPGP primary-key body             ✅
            ↓
    Fingerprint derivation               ✅
            ↓
    Certification digest                 ✅
            ↓
    Display UID + fingerprint            ✅
            ↓
    User approval                        ✅
            ↓
    Keycard certification signature      ✅
            ↓
    Positive UID certification packet    ✅
            ↓
    Certificate assembly                 ✅
            ↓
    Fingerprint / issuer binding          ✅
            ↓
    Cryptographic self-verification      ✅
            ↓
    Dedicated UR:BYTES request           ✅
            ↓
    UR:BYTES certificate response        ✅
            ↓
    Host request / response tooling       ✅
            ↓
    +------------------------------------+
    | PRODUCTION DERIVATION PATH         |
    |      ← WE ARE HERE / BLOCKED       |
    +------------------------------------+
            ↓
    OpenPGP menu/action                  ⬜
            ↓
    Physical Shell E2E                   ⬜
            ↓
    GnuPG final validation               ⬜


# Project Snapshot

    PR #225 — OpenPGP primitives
    ████████████████████████████████ 100% ✅
    Draft PR OPEN

    PR #226 — OpenPGP request protocol
    ████████████████████████████████ 100% ✅
    DRAFT PR OPEN / DEPENDS ON #225

    Identity orchestration core
    ████████████████████████████████ 100% ✅
    COMPLETE / BRANCH PUSHED / NO STANDALONE PR

    Derivation policy
    ████████████░░░░░░░░░░░░░░░░░░░  40% ⏳
    WAITING ON MAINTAINER

    Shell UI + QR integration
    ███████████████████████████░░░░░  85% 🟡
    CORE QR FLOW COMPLETE / MENU PATH PENDING

    Host E2E tooling
    ████████████████████████████████ 100% ✅
    REQUEST + RESPONSE QR ROUND TRIPS VERIFIED / PR OPEN

    Device + GnuPG E2E
    ███████████░░░░░░░░░░░░░░░░░░░░░  35% ⬜
    HOST TOOLING READY / WAITING ON PATH + PHYSICAL TEST

    Overall project
    ██████████████████████████░░░░░░  80% 🟡
    SOFTWARE + HOST VERTICAL SLICE READY / PHYSICAL E2E STILL PENDING


## Next Checkbox

- [x] host CREATE_IDENTITY request / response QR tooling
- [ ] maintainer selects / approves the production OpenPGP derivation path
- [ ] define that path in Shell and wire the OpenPGP menu/action
- [ ] run the first physical Shell CREATE_IDENTITY E2E
