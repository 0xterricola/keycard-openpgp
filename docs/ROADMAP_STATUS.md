# Keycard OpenPGP — Roadmap & Project Status

_Last updated: 2026-09-19_

This document tracks the current state of the air-gapped hardware signing project, the major moving pieces, and the next milestones.

## Status Legend

- ✅ Working / proven
- 🟡 Partially working / integration in progress
- ⏳ Not yet implemented
- 🔬 Research / architecture investigation
- ⚠️ Known issue

---

## 1. Application / Use Case — Thurin

Thurin is the first serious external application integration and gives the project a concrete end-to-end identity attestation target.

### Current status

- ✅ `@thurinlabs/identity-kit` supports secp256k1 PGP verification
- ✅ Thurin experiment updated to `identity-kit` 1.1.1
- ✅ All five existing Thurin/OpenPGP regression scripts pass on 1.1.1
- ✅ Hardware-signed attestation fixture still returns `{ verified: true }`
- ✅ React-free `/core` interface available
- ✅ Canonical attestation statement identified:
  - `I control the Ethereum address: 0x…`
- ✅ Thurin PGP verification path understood
- ✅ Thurin CLI source and signing boundaries inspected
- ✅ PGP secret material already remains outside Thurin; CLI shells out to `gpg`
- ✅ `--no-key` external-wallet handoff path understood
- ✅ `--authorize` EIP-712 path identified
- ✅ Registry contract identified:
  - `0x9302E02e2869e129aC8516fE5eFFd51EA3082c09`
- ✅ Thurin 0.7.0 external EIP-712 signer hook inspected and understood
- ✅ External signer contract identified:
  - typed EIP-712 data on stdin
  - 65-byte recoverable Ethereum signature on stdout
- ✅ `--sign-out` offline-signing handoff path understood
- ✅ `authorize finish` recovery, nonce-check, and simulation path understood
- ⏳ Produce a real 65-byte EIP-712 signature with the Keycard
- ⏳ Complete a real `--sign-out` → air gap → Keycard → `authorize finish` flow
- ⏳ Publish a hardware-authorized attestation on Sepolia
- ⏳ Verify the final result through the registry/browser lookup

### Desired Thurin flow

```text
Thurin
  ↓
build PGP attestation + EIP-712 authorization
  ↓
external signer interface
  ↓
air-gap middleware
  ↓
trusted hardware signer
  ↓
Keycard
  ↓
standard 65-byte recoverable Ethereum signature returned
  ↓
Thurin performs normal recovery / verification / simulation
  ↓
relayer or funded wallet pays gas
  ↓
registry
  ↓
browser lookup
```

---

## 2. Protocol Adapters — OpenPGP First

OpenPGP is the first protocol adapter, not the entire project.

### Current status

- ✅ NeoPGP/OpenPGP key generated on hardware card
- ✅ secp256k1 public key available
- ✅ OpenPGP fingerprint:
  - `31CE69D66A5E9DE0F977B59C79BB391497E8E6D4`
- ✅ Ethereum address derived from the same public point:
  - `0x9ce2e20fc392304fd1e50541ec67168913b5f3ff`
- ✅ Raw hardware ECDSA signing works
- ✅ Raw `r || s` signature returned from the card
- ⚠️ Current prototype signs `SHA-256(message)` directly; this is not yet a standards-compliant OpenPGP signature artifact
- ✅ Thurin's exact attestation message identified
- ⏳ Construct proper OpenPGP signature digest
- ⏳ Build standards-compliant ECDSA signature packet
- ⏳ Build full OpenPGP clearsigned artifact
- ⏳ Verify hardware-produced artifact with `identity-kit`
- ⏳ Define reusable external OpenPGP signing interface

### Future protocol adapters

- ⏳ EIP-712
- ⏳ Ethereum transactions
- ⏳ SSH
- ⏳ Nostr
- ⏳ Git / release signing
- ⏳ Other structured authorization formats

### Core design rule

Application-specific behavior must stay outside the trusted signer.

Protocol-specific behavior belongs in isolated protocol adapters rather than the generic signer core. Any protocol logic required to independently validate the displayed intent and construct the exact bytes or digest being signed must run inside the trusted-device boundary.

The signer must never blindly sign an opaque digest supplied by the online host when it cannot independently bind that digest to the human-readable intent.

---

## 3. Air-Gap Middleware / QR Transport

This is the layer between applications/protocol adapters and the trusted offline signer.

Mathom is a reference implementation to study for request → offline signer → response architecture.

Keycard Shell is the target device whose QR/signing transport needs to be studied and aligned with.

### Current status

- ✅ Camera reads QR codes
- ✅ Device generates response QR codes
- ✅ Prototype `KC1` request envelope works
- ✅ Prototype `KC1` response envelope works
- ✅ End-to-end disconnected QR request → hardware signing → QR response loop proven
- 🔬 Inspect Mathom request / response architecture
- 🔬 Inspect Keycard Shell QR and signing protocols
- 🔬 Determine BC-UR / multipart QR compatibility and fit
- ⏳ Generic signing-request envelope
- ⏳ Protocol/version identifiers
- ⏳ Request IDs / response correlation
- ⏳ Anti-replay / nonce strategy
- ✅ Prototype reject-over-truncate behavior proven for the current `KC1` message renderer
- ⏳ Define generic reject-over-truncate rendering rules for arbitrary protocols
- ⏳ Generic signing-response envelope
- ⏳ Middleware API for applications
- ⏳ Replace prototype-specific `KC1` transport with final format
- ⏳ Align transport with Keycard Shell

### Intended architecture

```text
Application
  ↓
Protocol adapter
  ↓
Air-gap middleware
  ↓
QR / BC-UR / transport
  ↓
════════ AIR GAP ════════
  ↓
Trusted signer
  ↓
Keycard
  ↓
signed response
  ↓
QR / transport
  ↓
Protocol adapter
  ↓
Application
```

---

## 4. Trusted Signer / Hardware Prototype

Current prototype hardware is the Microchip SAMA5D3 Xplained board.

Keycard Shell is the eventual target hardware.

### Working now

- ✅ SAMA5D3 prototype boots
- ✅ ST7789 trusted display works
- ✅ Camera works
- ✅ QR decoding works
- ✅ USB smartcard reader works
- ✅ NeoPGP card communication works
- ✅ Hardware signing operation works
- ✅ Signature-counter proof works
  - Request REJECT: counter unchanged
  - PIN-stage REJECT: counter unchanged
  - Request APPROVE + valid local PIN + PIN APPROVE: counter increments
- ✅ Signing intent can be shown on the trusted display
- ✅ Physical approval/rejection signing logic exists

### Keypad / local PIN

- ✅ Full 12-button matrix is electrically and logically correct
- ✅ Reliable `0–9` input
- ✅ Individually reliable APPROVE button
- ✅ Individually reliable REJECT button
- ✅ R1 moved from non-working PD30 path to PA16
- ✅ Local NeoPGP PIN entry through the physical keypad
- ✅ Masked PIN progress shown on the trusted display
- ✅ PIN digits are not echoed or logged
- ✅ PIN buffers are wiped on cancel/failure and after VERIFY use
- ✅ PIN-stage REJECT fails closed without signing
- ✅ One VERIFY attempt per signing attempt; failure exits closed
- ✅ Terminal PIN dependency removed
- ✅ Successful keypad PIN signing demonstrated with counter `10 → 11`
- ✅ PIN-stage cancellation demonstrated with counter `11 → 11`

### Final hardware hardening

- ⏳ Boot directly into signer application
- ⏳ Remove SSH dependency
- ⏳ Physically disconnect Ethernet
- ⏳ Final no-network / true-air-gap demonstration

### Temporary development mode

The working SAMA signer no longer depends on the SSH terminal for PIN entry.

SSH is still being used as a development and control channel to launch and debug the signer.

Booting directly into the signer and physically removing networking are still required before claiming the final device is fully air-gapped.

---

## 5. Target Hardware — Keycard Shell

The SAMA board is the prototype platform.

Keycard Shell is the target device for the portable/general implementation.

### Current status

- 🔬 Study Shell application architecture
- 🔬 Study Shell QR transport
- 🔬 Study Shell smartcard APIs
- 🔬 Study display/input APIs
- 🔬 Map trusted signer core onto Shell
- ⏳ Port protocol adapters
- ⏳ Port QR middleware
- ⏳ Port trusted intent renderer
- ⏳ Port approval / PIN UI
- ⏳ Integrate NeoPGP / Keycard on Shell
- ⏳ Prove hardware-independent signer architecture

---

## 6. General Architecture

```text
APPLICATIONS
Thurin / Git / SSH / Nostr / future apps
                │
                ▼
PROTOCOL ADAPTERS
OpenPGP / EIP-712 / Ethereum / SSH / ...
                │
                ▼
AIR-GAP MIDDLEWARE
Mathom concepts / QR / BC-UR / request-response
                │
          ═════ AIR GAP ═════
                │
                ▼
TRUSTED SIGNER CORE
parse → validate → render → approve/reject → PIN
                │
                ▼
HARDWARE KEY
NeoPGP / Keycard
                │
                ▼
SAMA5D3 prototype → Keycard Shell target
```

The project goal is not a Thurin signer or an OpenPGP-only device.

The goal is a reusable trusted air-gapped signing system where applications hand over structured intents, the user can understand exactly what is being authorized, and the private key never leaves the hardware card.

---

## 7. Immediate Priorities

### Recently completed

- ✅ Completed keypad electrical and matrix debugging
- ✅ All 12 physical keypad buttons working under Linux
- ✅ Replaced the non-working R1 / PD30 path with PA16
- ✅ Implemented local NeoPGP PIN entry
- ✅ Added masked PIN progress to the trusted display
- ✅ Verified PIN-stage cancellation fails closed
- ✅ Removed the terminal PIN dependency
- ✅ Copied the working SAMA signer sources into the repository under `embedded/sama5d3/`
- ✅ Documented the broader project roadmap and current status
- ✅ Updated Thurin experiment to current `identity-kit` 1.1.1
- ✅ Verified Thurin 0.7.0 external EIP-712 signer / sign-out integration path
- ⏳ Implement the standards-compliant OpenPGP signing path on top of the proven raw hardware ECDSA primitive
- 🔬 Keep Mathom / Keycard Shell transport research queued as the middleware layer becomes more concrete

### Next major milestones

1. ✅ Get all 12 keypad buttons electrically and logically correct
2. ✅ Implement trusted local PIN entry with masked on-device feedback
3. ⏳ Produce a standards-compliant OpenPGP signed artifact from the hardware signature
4. ⏳ Make `identity-kit` verify that artifact successfully
5. ⏳ Define / adopt the generic QR signing middleware
6. ⏳ Add Keycard-backed EIP-712 authorization for Thurin
7. ⏳ Publish and verify the Sepolia attestation
8. ⏳ Remove Ethernet / SSH and demonstrate the complete air-gapped flow

---

## 8. Proven Milestones

- ✅ NeoPGP secp256k1 key generated on-card
- ✅ Private key never exported
- ✅ Same public point mapped to OpenPGP identity and Ethereum address
- ✅ Trusted display operational
- ✅ Camera-driven QR request works
- ✅ End-to-end disconnected QR request → hardware signing → QR response loop proven
- ✅ Physical APPROVE / REJECT decision affects signing
- ✅ Full 3×4 physical keypad operational under Linux
- ✅ Local NeoPGP PIN entry works through the trusted keypad
- ✅ PIN progress is masked on the trusted display
- ✅ PIN-stage cancellation fails closed with signature counter unchanged (`11 → 11`)
- ✅ Successful local-PIN signing increments the signature counter (`10 → 11`)
- ✅ Terminal PIN entry dependency removed
- ✅ Signature counter proves request rejection and PIN cancellation do not sign
- ✅ Signature counter proves approved request + valid local PIN produces a hardware signature
- ✅ Raw signature returned via QR
- ✅ Thurin's PGP and Ethereum signing boundaries understood
- ✅ secp256k1 verification support landed upstream in Thurin identity tooling
- ✅ Ben's Thurin compatibility PR merged
- ✅ Thurin experiment upgraded and regression-tested on `identity-kit` 1.1.1
- ✅ Repository licensed under MIT

---

## 9. Guiding Security Properties

- The private key stays on the hardware card.
- The trusted device must display what is actually being signed.
- The displayed intent must be cryptographically bound to the signed bytes.
- Unsupported or unrenderable requests must be rejected, not truncated or guessed.
- PIN entry belongs on the trusted device.
- Failed PIN verification must fail closed.
- The trusted device must independently bind human-readable intent to the exact bytes or digest authorized for signing.
- The signer must not blindly sign opaque host-supplied digests that it cannot independently validate against the displayed intent.
- PIN values must never be displayed, logged, or returned over the transport.
- Applications should receive standard cryptographic artifacts rather than hardware-specific formats.
- Protocol-specific encoding belongs outside the trusted signer core.
- The online application should not need to know which hardware produced a valid signature.
