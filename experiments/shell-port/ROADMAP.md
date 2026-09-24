# Keycard Shell — OpenPGP Identity Project

## Overall Project

`█████████████████░░░░░░░░░░░░░░  ~55%`

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

# Planned PR — OpenPGP Request Protocol

**Branch:** `feature/openpgp-integration`

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
- [ ] branch pushed
- [ ] PR opened
- [ ] reviewed / merged

**Next:** Push the branch and open a focused request-protocol PR.


---

# Planned PR — OpenPGP Identity Orchestration

**Branch:** `feature/openpgp-orchestration`

**Current active branch**

`████████████████░░░░░░░░░░░░░░  ~50% 🟡 IN PROGRESS`

## Completed Commits

- `a2bd3fe` — `openpgp: prepare primary key from Keycard path`
- `28fa692` — `openpgp: prepare UID certification digest`

## Completed

- [x] explicit path passed into OpenPGP orchestration
- [x] no mutation of wallet-oriented `g_core.bip44_path`
- [x] Keycard public-key export
- [x] 65-byte secp256k1 public point
- [x] 79-byte OpenPGP v4 primary-key body
- [x] 20-byte OpenPGP fingerprint
- [x] UID certification preimage
- [x] v4 signature fields
- [x] exact SHA-256 certification digest

## Current Pipeline

    Shell-selected path
            ↓
    Keycard EXPORT KEY                    ✅
            ↓
    secp256k1 public point                ✅
            ↓
    OpenPGP primary-key body              ✅
            ↓
    OpenPGP fingerprint                  ✅
            ↓
    UID + creation_time                  ✅
            ↓
    certification preimage               ✅
            ↓
    signature fields                     ✅
            ↓
    SHA-256 certification digest         ✅
            ↓
    ----------------------------------------
    UID + FINGERPRINT APPROVAL            ← NEXT
    ----------------------------------------
            ↓
    physical user confirmation           ⬜
            ↓
    Keycard signs digest                 ⬜
            ↓
    decode r || s                        ⬜
            ↓
    build OpenPGP signature packet       ⬜
            ↓
    assemble complete certificate        ⬜
            ↓
    self-verify certificate              ⬜
            ↓
    encode response as UR:BYTES          ⬜
            ↓
    display response QR                  ⬜

## Expected Next Commits

    NEXT
    openpgp: confirm identity before certification

    THEN
    openpgp: sign UID certification with Keycard

    THEN
    openpgp: assemble and verify identity certificate

    THEN
    openpgp: add QR identity creation flow

## Status

- [x] branch exists locally
- [x] first two orchestration commits complete
- [x] repeated firmware builds passing
- [ ] approval UI
- [ ] signing
- [ ] certificate assembly
- [ ] QR flow
- [ ] PR opened


---

# Derivation Policy

**Purpose:** Final Shell-owned OpenPGP derivation path

`██████░░░░░░░░░░░░░░░░░░░░░░  ~20% ⏳ BLOCKED ON GUIDANCE`

## Completed

- [x] host does not control derivation path
- [x] experimental Ethereum path rejected for production
- [x] SLIP-0013 investigated
- [x] SLIP-0017 investigated
- [x] ERC-1581 / Keycard namespace investigated
- [x] path decision isolated from orchestration plumbing
- [x] maintainer question sent

## Remaining

- [ ] exact production namespace/path
- [ ] define path constants/helper
- [ ] wire final policy into identity workflow
- [ ] document rationale

## Expected Commit

    openpgp: define identity derivation path

**Important:** This does not block the current approval/signing plumbing.

The current APIs accept an explicit path so the final namespace can be inserted later without redesigning the feature.


---

# Shell UI + QR Integration

`████████░░░░░░░░░░░░░░░░░░░░  ~25% 🟡`

## Completed

- [x] existing `ui_qrscan()` supports explicit UR type
- [x] BYTES transport already available
- [x] existing QR output supports explicit BYTES
- [x] decided not to add generic BYTES to `UR_ANY_TX`
- [x] dedicated OpenPGP action architecture identified

## Remaining

- [ ] OpenPGP menu/action
- [ ] `ui_qrscan(BYTES, ...)`
- [ ] parse OpenPGP request
- [ ] show UID
- [ ] show fingerprint
- [ ] cancellable physical approval
- [ ] execute approved identity workflow
- [ ] output certificate via BYTES QR

## Security Rule

Generic QR scanning must not accidentally turn arbitrary `UR:BYTES`
payloads into OpenPGP signing requests.

OpenPGP gets a dedicated, purpose-specific action.


---

# Final Device + GnuPG Validation

`██████░░░░░░░░░░░░░░░░░░░░░░  ~20% ⬜`

## Already Proven

- [x] original Keycard applet experiment
- [x] Keycard-derived secp256k1 key
- [x] Keycard signs OpenPGP certification digest
- [x] assembled certificate accepted by GnuPG
- [x] valid self-signature demonstrated
- [x] primitive host harness
- [x] request-protocol host harness
- [x] repeated production firmware builds
- [x] repeated test firmware builds

## Final Shell E2E Still Required

- [ ] generate CREATE_IDENTITY request
- [ ] scan request with physical Shell
- [ ] Shell derives physical Keycard public key
- [ ] Shell displays requested UID
- [ ] Shell displays derived fingerprint
- [ ] user physically approves
- [ ] Keycard signs certification digest
- [ ] Shell constructs certificate
- [ ] Shell verifies certificate before returning it
- [ ] Shell displays certificate as response QR
- [ ] host scans response
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


---

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
       +-- CODE COMPLETE ✅
       |   PR NOT OPEN YET
       |
       v
    feature/openpgp-orchestration
    a2bd3fe
    28fa692
       |
       +-- CURRENT BRANCH 🟡
           MORE COMMITS COMING


---

# Current Exact Position

    Request protocol                     ✅
            ↓
    UID + creation_time                  ✅
            ↓
    Shell-owned path interface           ✅
            ↓
    Keycard public-key export            ✅
            ↓
    OpenPGP primary-key body             ✅
            ↓
    Fingerprint derivation               ✅
            ↓
    Certification digest                 ✅
            ↓
    +------------------------------------+
    | DISPLAY UID + FINGERPRINT          |
    |          ← WE ARE HERE             |
    +------------------------------------+
            ↓
    User approval                        ⬜
            ↓
    Keycard certification signature      ⬜
            ↓
    Certificate assembly                 ⬜
            ↓
    Self-verification                    ⬜
            ↓
    UR:BYTES response                    ⬜
            ↓
    Physical Shell E2E                   ⬜
            ↓
    GnuPG validation                     ⬜


---

# Project Snapshot

    PR #225 — OpenPGP primitives
    ████████████████████████████████ 100% ✅
    Draft PR OPEN

    Planned PR — Request protocol
    ████████████████████████████████ 100% ✅
    CODE COMPLETE / PR NOT OPEN

    Planned PR — Identity orchestration
    ████████████████░░░░░░░░░░░░░░  50% 🟡
    CURRENT ACTIVE WORK

    Derivation policy
    ██████░░░░░░░░░░░░░░░░░░░░░░  20% ⏳
    WAITING ON MAINTAINER

    Device + GnuPG E2E
    ██████░░░░░░░░░░░░░░░░░░░░░░  20% ⬜
    PENDING COMPLETE WORKFLOW


---

## Next Checkbox

- [ ] UID + derived fingerprint approval UI
