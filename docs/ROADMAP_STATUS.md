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
- 🟡 Collaborating around an external signer hook for `--authorize`
- ⏳ Plug Keycard / air-gapped EIP-712 signing into Thurin
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
standard signature returned
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

Protocol-specific logic belongs in protocol adapters. The trusted signer core should not contain Thurin-specific or application-specific behavior.

---

## 3. Air-Gap Middleware / QR Transport

This is the layer between applications/protocol adapters and the trusted offline signer.

Mathom is a reference for the asynchronous request → offline signer → response pattern.

Keycard Shell is the target device whose QR/signing transport needs to be studied and aligned with.

### Current status

- ✅ Camera reads QR codes
- ✅ Device generates response QR codes
- ✅ Prototype `KC1` request envelope works
- ✅ Prototype `KC1` response envelope works
- ✅ End-to-end asynchronous QR signing loop proven
- 🔬 Inspect Mathom request / response architecture
- 🔬 Inspect Keycard Shell QR and signing protocols
- 🔬 Determine BC-UR / multipart QR compatibility and fit
- ⏳ Generic signing-request envelope
- ⏳ Protocol/version identifiers
- ⏳ Request IDs / response correlation
- ⏳ Anti-replay / nonce strategy
- ⏳ Reject-over-truncate rendering rules
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
  - REJECT: counter unchanged
  - APPROVE: counter increments
- ✅ Signing intent can be shown on the trusted display
- ✅ Physical approval/rejection signing logic exists

### Keypad / local PIN

- 🟡 12-button matrix driver loads
- 🟡 R4 / PE10 physically responds
- ⚠️ REJECT / `0` / APPROVE currently report together
- ⚠️ R1 / R2 / R3 are not producing expected interrupts
- ⏳ Finish continuity / electrical debugging
- ⏳ Reliable `0–9` input
- ⏳ Individually reliable APPROVE button
- ⏳ Individually reliable REJECT button
- ⏳ Local PIN entry
- ⏳ PIN buffer wiping
- ⏳ One-attempt VERIFY / fail-closed behavior
- ⏳ Remove terminal PIN dependency

### Final hardware hardening

- ⏳ Boot directly into signer application
- ⏳ Remove SSH dependency
- ⏳ Physically disconnect Ethernet
- ⏳ Final no-network / true-air-gap demonstration

### Temporary development mode

Terminal PIN entry can remain temporarily while the OpenPGP and middleware layers are developed.

Local PIN entry and removal of networking are required before claiming the final signer is air-gapped.

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

### Today / current development session

- 🟡 Continue keypad hardware debugging
- 🟡 Improve hardware-side progress toward local PIN input
- ✅ Document the broader project roadmap and current status
- 🟡 Keep Thurin integration architecture current as upstream evolves
- 🟡 Track the OpenPGP artifact work needed for the next interoperability milestone
- 🔬 Keep Mathom / Keycard Shell transport research queued as the middleware layer becomes more concrete

### Next major milestones

1. ⏳ Get all 12 keypad buttons electrically and logically correct
2. ⏳ Implement local PIN entry
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
- ✅ Physical APPROVE / REJECT decision affects signing
- ✅ Signature counter proves reject does not sign and approve does
- ✅ Raw signature returned via QR
- ✅ Thurin's PGP and Ethereum signing boundaries understood
- ✅ secp256k1 verification support landed upstream in Thurin identity tooling
- ✅ Repository licensed under MIT

---

## 9. Guiding Security Properties

- The private key stays on the hardware card.
- The trusted device must display what is actually being signed.
- The displayed intent must be cryptographically bound to the signed bytes.
- Unsupported or unrenderable requests must be rejected, not truncated or guessed.
- PIN entry belongs on the trusted device.
- Failed PIN verification must fail closed.
- Applications should receive standard cryptographic artifacts rather than hardware-specific formats.
- Protocol-specific encoding belongs outside the trusted signer core.
- The online application should not need to know which hardware produced a valid signature.
