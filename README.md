# keycard-openpgp

Experimental trusted signing architecture using removable hardware keys, disconnected transport, and human-verifiable authorization, with OpenPGP as the first protocol adapter and Keycard Shell as the target portable interface.

## Project Goal

`keycard-openpgp` is an experimental trusted signing architecture built around a removable hardware key and a disconnected human-verification device.

The goal is not merely to make a JavaCard sign hashes. An application should hand over structured intent to a trusted device that independently validates and renders what is being authorized, requires physical approval and local PIN entry, binds that intent to the exact bytes or digest being signed, and returns a standard cryptographic artifact.

OpenPGP is the first protocol adapter. Thurin is the first external application integration. The SAMA5D3 is the reference prototype. Keycard Shell is the intended portable target.

## Initial Research Question

The project began by asking whether one JavaCard-held `secp256k1` key could back an OpenPGP identity while the same public point also defines an Ethereum address.

That experiment succeeded, but shared-key use across protocols is not a requirement of the general architecture. A production design may intentionally separate keys and security domains.

## Current Prototype Status

The SAMA5D3 reference prototype now demonstrates the trusted interaction loop around the NeoPGP hardware signing primitive.

### Proven on the SAMA5D3

- ✅ camera-driven QR request input
- ✅ prototype `KC1` request parsing
- ✅ trusted display of signing intent
- ✅ independent physical APPROVE / REJECT controls
- ✅ full 3×4 numeric keypad
- ✅ local NeoPGP PIN entry
- ✅ masked PIN feedback on the trusted display
- ✅ request rejection causes no signing operation
- ✅ PIN-stage cancellation causes no signing operation
- ✅ NeoPGP secp256k1 private-key operation
- ✅ raw 64-byte `r || s` ECDSA signature
- ✅ QR signature response
- ✅ disconnected QR request → hardware signing → QR response loop

### Still in progress

- ⏳ standards-compliant OpenPGP artifact from the embedded signer
- ⏳ hardware-produced OpenPGP artifact verified by `identity-kit`
- ⏳ Keycard-backed EIP-712 / 65-byte recoverable Ethereum signing
- ⏳ generic replay-safe request/response middleware
- ⏳ network-free boot/runtime
- ⏳ Keycard Shell port

### Hardware

- HID Global OMNIKEY 3x21 reader
- NXP programmable JavaCard originally used in the PhononDAO alpha
- NeoPGP installed
- Microchip SAMA5D3 Xplained
- ST7789 trusted display
- USB camera
- 3×4 physical keypad

The current JavaCard is a programmable research card. It is not a retail Keycard.

## NeoPGP + secp256k1

NeoPGP was installed with secp256k1 enabled:

```bash
gp -install NeoPGPApplet.cap \
  -params 02000000 \
  -key 404142434445464748494A4B4C4D4E4F
```

GnuPG recognizes:

```text
Application type .: OpenPGP
Version ..........: 3.4
Manufacturer .....: NeoPGP
Key attributes ...: secp256k1 secp256k1 secp256k1
```

`gpg-card` rejects secp256k1 from its key-generation allowlist, but direct `scdaemon` generation works:

```bash
gpg-connect-agent "SCD GENKEY OPENPGP.1" /bye
```

The private key was generated on-card and has never been exported.

## Research Identity

OpenPGP fingerprint:

```text
31CE69D66A5E9DE0F977B59C79BB391497E8E6D4
```

Ethereum address derived from the same secp256k1 public point:

```text
0x9cE2E20FC392304fD1e50541eC67168913B5f3fF
```

Therefore:

```text
ONE JavaCard-held secp256k1 private key
                ↓
        secp256k1 public point
           ↙             ↘
 OpenPGP identity     Ethereum address
```

The Ethereum address is unfunded and experimental.

## OpenPGP

GnuPG successfully created a normal OpenPGP certificate around the existing card key.

Test UID:

```text
terricola-testtt
```

Hardware-backed signing works and verifies successfully with GnuPG.

That connected GnuPG path produces normal OpenPGP signatures through the smart-card stack.

The current SAMA5D3 direct-signing prototype operates at a lower layer: it currently signs `SHA-256(message)` with the NeoPGP ECDSA key and receives the raw 64-byte `r || s` result.

Turning that proven raw hardware primitive into a standards-compliant OpenPGP signature packet and clearsigned or detached artifact is the next protocol milestone.

## Thurin Integration

Thurin is the first serious external application integration for the architecture.

`@thurinlabs/identity-kit` 1.1.1 successfully parses and verifies the existing hardware-backed `secp256k1` OpenPGP fixture.

The existing OpenPGP regression tests pass on 1.1.1, and the hardware-backed attestation fixture returns:

```text
{ verified: true }
```

The canonical Thurin attestation statement is:

```text
I control the Ethereum address: 0x…
```

The existing fixture demonstrates that the OpenPGP key can attest to the Ethereum address derived from the same public point.

### Thurin 0.7.0 external signer interface

Thurin 0.7.0 exposes the EIP-712 authorization seam needed for a hardware signer.

Its external signer interface sends EIP-712 typed data as JSON to the signer and expects a 65-byte recoverable Ethereum signature in response.

Thurin also supports a disconnected signing handoff:

```text
Thurin --sign-out
        ↓
typed-data handoff
        ↓
offline / external signer
        ↓
65-byte signature
        ↓
Thurin authorize finish
```

The `authorize finish` path performs signer recovery, checks the current registry nonce, and simulates the matching authorization call before continuing.

The real Keycard-backed EIP-712 signature has not yet been produced.

### Mathom and the air-gap workflow

Mathom is reference work for studying the host → offline signer → response boundary.

The project does not currently claim asynchronous middleware semantics and does not assume Mathom's QR encoding will be reused directly.

Keycard Shell already provides useful QR primitives, while BC-UR and multipart transport remain candidates for the eventual transport layer.

The current proven prototype instead uses the simpler `KC1` request/response format.

## Embedded Linux Reference Prototype

A Microchip SAMA5D3 Xplained board is now being used as a reference platform for the eventual trusted-device flow.

The board runs Buildroot Linux and includes:

- `pcsc-lite`
- `libusb`
- `ccid`
- OpenSC

Current hardware path:

```text
Mac
 ↓ Ethernet / SSH
SAMA5D3 Xplained
 ↓ USB
HID Global OMNIKEY 3x21
 ↓ ISO-7816
JavaCard / NeoPGP
```

The embedded board successfully detects the reader through PC/SC:

```text
Nr.  Card  Features  Name
0    Yes             HID Global OMNIKEY 3x21 Smart Card Reader
```

### NeoPGP Selection

The board successfully selects the NeoPGP application AID:

```text
D2760001240103040010000000000000
```

APDU response:

```text
90 00
```

The OpenPGP application identifier can then be read directly from the card:

```text
D2 76 00 01 24 01 03 04 00 10 00 00 00 00 00 00
```

### Signing-Key Fingerprint

The embedded board reads the signing-key fingerprint directly from NeoPGP:

```text
31 CE 69 D6 6A 5E 9D E0 F9 77 B5 9C 79 BB 39 14
97 E8 E6 D4
```

This exactly matches the key previously used for the desktop OpenPGP and Ethereum identity experiments.

### secp256k1 Confirmation

The card reports the signing algorithm attributes:

```text
13 2B 81 04 00 0A FF
```

This identifies the signing key as ECDSA over `secp256k1`.

### Embedded Hardware Signature

The SAMA5D3 successfully performed a real signing operation through NeoPGP.

Test message:

```text
keycard-openpgp embedded signing test
```

Flow:

```text
SELECT NeoPGP
      ↓
    90 00

VERIFY signing PIN
      ↓
    90 00

PSO: COMPUTE DIGITAL SIGNATURE
      ↓
64-byte ECDSA signature
      ↓
    90 00
```

The returned ECDSA signature was independently verified on the Mac against the known secp256k1 public key.

Result:

```text
VALID: embedded NeoPGP secp256k1 signature verified
```

This proves the complete reference path:

```text
SAMA5D3 Linux
      ↓
pcsc-lite / libusb / CCID
      ↓
OMNIKEY
      ↓
NeoPGP JavaCard
      ↓
secp256k1 private-key operation
      ↓
ECDSA signature
      ↓
independent host verification
```

The private key never leaves the JavaCard.


### Trusted interaction prototype

The SAMA5D3 reference platform now also proves the human-authorization path around the hardware private-key operation.

```text
QR signing request
        ↓
camera
        ↓
request parsing
        ↓
trusted display
        ↓
physical APPROVE / REJECT
        ↓
local masked PIN entry
        ↓
NeoPGP VERIFY
        ↓
hardware ECDSA operation
        ↓
QR signing response
```

The 3×4 keypad provides:

```text
1 2 3
4 5 6
7 8 9
REJECT 0 APPROVE
```

Request-level rejection has been demonstrated with the NeoPGP signature counter unchanged.

PIN-stage cancellation has also been demonstrated with the counter unchanged (`11 → 11`).

A successful local-PIN signing operation incremented the counter (`10 → 11`).

PIN digits are never echoed to the terminal and are represented only by masked progress on the trusted display.

SSH is still used to launch and debug the prototype, so the current SAMA5D3 system is not yet claimed as a final physically air-gapped device.

## Keycard Shell

Keycard Shell remains the target hardware interface.

The Shell firmware already provides useful primitives for this direction:

- ISO-7816 smartcard transport
- display
- physical controls
- camera
- QR support

An experimental Shell firmware branch adds detection of the NeoPGP applet by attempting to select:

```text
D2760001240103040010000000000000
```

The modified Shell firmware builds successfully.

The current retail Shell bootloader only accepts appropriately signed firmware, so this custom firmware has not yet been run on the retail device.

The embedded Linux board is therefore being used as a reference platform for developing the card API, structured request format, trusted-display semantics, physical approval flow, and QR protocol before moving those ideas onto Shell.

## Human-Readable Signing Intent

Human-readable intent is a core security property of the architecture.

The trusted device must independently understand enough about the requested operation to render what the human is actually authorizing.

The central rule is:

> The human-readable display must be cryptographically bound to the exact bytes or digest that is ultimately authorized for signing.

The online host must not be able to provide friendly display text while separately supplying an unrelated opaque digest for the hardware key to sign.

If the trusted device cannot independently validate the relationship between displayed intent and signed material, it must reject the request.

The trusted side therefore owns the security-critical path:

```text
structured request
        ↓
parse + validate
        ↓
trusted rendering
        ↓
physical approval
        ↓
protocol-specific trusted encoding
        ↓
exact signed bytes / digest
        ↓
hardware key
```

Potential protocol adapters include:

```text
pgp_cleartext_sign
pgp_certify_key
pgp_eth_binding
eip712_sign
ethereum_transaction
ssh_sign
nostr_sign
git_commit_sign
file_sign
```

## Disconnected Air-Gapped Signing Flow

The current reference prototype demonstrates a disconnected, sequential request/response signing flow:

```text
host prepares structured request
             ↓
          QR request
             ↓

           AIR GAP

             ↓
trusted device parses request
             ↓
human-readable display
             ↓
physical approval
             ↓
local PIN
             ↓
hardware signature
             ↓
         response QR
             ↓

           AIR GAP

             ↓
host verifies / consumes result
```

This disconnected QR signing loop is proven.

A generalized asynchronous middleware system is not yet implemented.

Persistent pending requests, multiple outstanding operations, request correlation, retries, multipart transport, and replay protection remain future middleware work.

Mathom remains useful reference work for the host/offline-signer boundary, while Keycard Shell QR support and BC-UR remain candidates for the eventual transport layer.

## Proven

- ✅ NeoPGP runs on the JavaCard
- ✅ `secp256k1` key generation works on-card
- ✅ private key remains on-card
- ✅ GnuPG recognizes the card as OpenPGP 3.4
- ✅ connected GnuPG hardware OpenPGP signing works
- ✅ the same public point derives an Ethereum address
- ✅ Thurin `identity-kit` verifies the existing hardware-backed OpenPGP fixture
- ✅ SAMA5D3 detects the OMNIKEY reader through PC/SC
- ✅ embedded Linux selects the NeoPGP applet
- ✅ embedded Linux reads the correct signing-key fingerprint
- ✅ embedded Linux performs a real hardware private-key operation
- ✅ raw embedded ECDSA signatures verify against the known public key
- ✅ trusted signing-intent display works
- ✅ full 3×4 physical keypad works
- ✅ physical APPROVE / REJECT controls work
- ✅ local NeoPGP PIN entry works
- ✅ masked PIN progress works on the trusted display
- ✅ request rejection performs no signing operation
- ✅ PIN-stage cancellation performs no signing operation
- ✅ QR request input works
- ✅ QR signature response works
- ✅ disconnected QR request → hardware signing → QR response loop works
- ✅ experimental Keycard Shell NeoPGP detection firmware builds
- ✅ Thurin 0.7.0 external EIP-712 signer interface is understood

## Not Yet Proven

- ⏳ standards-compliant OpenPGP artifact from the embedded direct-signing path
- ⏳ embedded hardware OpenPGP artifact verified by `identity-kit`
- ⏳ 65-byte recoverable Ethereum signature produced by the Keycard
- ⏳ real Thurin `--sign-out` → hardware signer → `authorize finish` flow
- ⏳ hardware-authorized Thurin attestation on Sepolia
- ⏳ generic replay-safe signing middleware
- ⏳ request IDs and response correlation
- ⏳ BC-UR / multipart transport
- ⏳ boot directly into the signer without SSH
- ⏳ physically network-disconnected runtime demonstration
- ⏳ NeoPGP signing through Keycard Shell
- ⏳ production backup / recovery design
- ⏳ production policy for cross-protocol key use

## Next Milestone: Standards-Compliant OpenPGP Artifact

The trusted hardware interaction loop is working.

The next protocol milestone is to turn the proven raw NeoPGP ECDSA primitive into a standards-compliant OpenPGP signed artifact.

Target flow:

```text
structured signing request
        ↓
trusted device validates + renders intent
        ↓
physical approval + local PIN
        ↓
construct correct OpenPGP signature digest
        ↓
NeoPGP ECDSA private-key operation
        ↓
build OpenPGP ECDSA signature packet
        ↓
build clearsigned / detached artifact
        ↓
identity-kit verification
```

After that:

1. verify the hardware-produced OpenPGP artifact with `identity-kit`,
2. implement the Keycard-backed EIP-712 adapter,
3. complete a real Thurin `--sign-out` → hardware signer → `authorize finish` flow,
4. define the reusable replay-safe QR middleware,
5. remove SSH / Ethernet and demonstrate the network-free reference flow,
6. port the proven interaction model onto Keycard Shell.

The larger research question is:

> What should a general-purpose hardware signing interface look like when applications provide structured intent, the trusted device independently validates and renders what is being authorized, and a removable hardware key performs the private-key operation across a disconnected transport?

## Security

Experimental research only.

Do not use this prototype for meaningful funds, production identities, or sensitive long-lived keys.

Using one private key across OpenPGP and Ethereum collapses security domains. A production design needs strict protocol separation, trusted display, and explicit physical authorization.

No private keys, PINs, local GnuPG state, firmware signing keys, or other sensitive development credentials should be committed to this repository.

## Documentation

- [Roadmap & Project Status](docs/ROADMAP_STATUS.md)

- [End-to-End QR Signing Prototype](docs/END_TO_END_QR_SIGNING.md) — optical signing requests, trusted display review, physical approve/reject gating, NeoPGP hardware signing, and QR responses.
