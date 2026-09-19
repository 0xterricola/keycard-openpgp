# keycard-openpgp

Experimental OpenPGP work on programmable JavaCards, targeting Keycard Shell as an air-gapped hardware interface.

## Thesis

Can a removable JavaCard hold a hardware-backed OpenPGP identity while the same `secp256k1` public key also deterministically defines an Ethereum address?

Longer-term:

```text
GnuPG / Thurin CLI
        ↓
structured signing intent
        ↓
animated QR
        ↓
Keycard Shell
display + keypad + camera
        ↓
ISO-7816
        ↓
JavaCard / NeoPGP
        ↓
non-exportable private key
```

The broader goal is to explore Keycard Shell as a general-purpose air-gapped cryptographic interface, with OpenPGP as the first non-wallet protocol.

The important UX and security question is not merely how to sign a hash, but how the trusted device can show the human what they are actually authorizing before the signature occurs.

## Current Status

The core cryptographic proof-of-concept now works end-to-end on both a desktop host and an embedded Linux reference platform.

### Hardware

- HID Global OMNIKEY 3x21 reader
- NXP JavaCard originally used in the PhononDAO alpha
- GlobalPlatform 2.1.1 / SCP02
- JavaCard 3.0.4 compatible
- Original Phonon applet removed
- NeoPGP installed
- Microchip SAMA5D3 Xplained embedded Linux reference platform

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

## Thurin Integration

`@thurinlabs/identity-kit` successfully parses the certificate:

```text
fingerprint:
31CE69D66A5E9DE0F977B59C79BB391497E8E6D4

userIDs:
terricola-testtt

algorithm:
secp256k1
```

Two OpenPGP.js compatibility details were found, and both are now handled inside identity-kit
(1.0.2 and later):

- OpenPGP.js rejects secp256k1 by default (`rejectCurves`), because RFC 9580 does not list the
  curve. identity-kit clears that rejection on every verification path, so no `config` is needed
  by callers.
- In Node, OpenPGP.js needs `eckey-utils` for this curve. identity-kit declares it as a
  dependency, so `npm install` brings it in. The browser build needs nothing extra.

With `@thurinlabs/identity-kit@^1.0.2`, `verifyAttestation()` returns for the complete
hardware-signed attestation, unmodified:

```text
{ verified: true }
```

The attestation in `experiments/thurin/` is also the secp256k1 test fixture in identity-kit's own
test suite, so this key type stays covered there.

The tested attestation binds the OpenPGP key to the Ethereum address derived from the same public point.

### Mathom and the air-gap workflow

Mathom is also relevant to the next stage of this project.

Mathom explores an air-gapped OpenPGP architecture where cryptographic requests cross a QR boundary to an offline signer rather than requiring the private-key environment to remain directly connected to the host.

That makes it useful reference work for several problems this project now needs to solve:

- preparing an OpenPGP operation on the host
- moving the request across an air gap
- executing the sensitive operation on the offline side
- returning the result to the host
- preserving enough state for an asynchronous workflow

The goal is not necessarily to reuse Mathom's QR encoding directly.

Keycard Shell already has animated QR support, including BC-UR / ERC-4527. Mathom can instead inform the OpenPGP application layer and the host/offline-signer boundary, while Shell provides the eventual trusted hardware interface and QR transport.

Conceptually:

```text
GnuPG / Thurin / host application
              ↓
     asynchronous request
              ↓
        QR transport
              ↓

           AIR GAP

              ↓
     trusted signing device
              ↓
    human-readable intent
              ↓
     physical authorization
              ↓
       NeoPGP JavaCard
              ↓
       signed response
              ↓
        QR back to host
```

The work in this repository is focused particularly on the layer between those pieces: defining structured signing intent, rendering that intent on a trusted display, and ensuring the data shown to the human is cryptographically tied to what the JavaCard actually signs.


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

The next problem is not simply making another device sign arbitrary hashes.

The goal is for the trusted device to understand enough about the operation to show the human what is actually being authorized.

For example:

```text
PGP ↔ Ethereum Binding

PGP fingerprint:
31CE69D6...97E8E6D4

Ethereum address:
0x9cE2E20F...13B5f3fF

Statement:
I control this Ethereum address

Reject              Approve
```

The important security property is:

> The human-readable display must be derived from the same structured data that is ultimately signed.

The host should not be able to provide friendly display text that is unrelated to an opaque hash being authorized.

A future request could therefore describe a typed operation such as:

```text
operation:
  pgp_eth_binding

pgp_fingerprint:
  31CE69D66A5E9DE0F977B59C79BB391497E8E6D4

ethereum_address:
  0x9cE2E20FC392304fD1e50541eC67168913B5f3fF

statement:
  I control Ethereum address
  0x9cE2E20FC392304fD1e50541eC67168913B5f3fF
```

The trusted device can validate that structure, render it itself, require physical approval, and only then ask the JavaCard to sign.

Potential future intent types include:

```text
pgp_cleartext_sign
pgp_certify_key
pgp_eth_binding
git_commit_sign
file_sign
```

## Asynchronous Air-Gapped Flow

Animated QR is intended to replace the permanently connected smart-card assumption.

Conceptually:

```text
host prepares structured request
             ↓
       animated QR
             ↓

          AIR GAP

             ↓
trusted device parses request
             ↓
human-readable display
             ↓
physical approval
             ↓
NeoPGP signature
             ↓
       response QR
             ↓

          AIR GAP

             ↓
host verifies / consumes result
```

Mathom provides useful reference work for the asynchronous OpenPGP request/response model, while Keycard Shell's existing BC-UR / ERC-4527 support is a natural candidate for the transport layer rather than inventing a new framing protocol.

The remaining GPG integration problem is making the host-side workflow asynchronous: the signing request leaves the host, is approved and signed elsewhere, and the response returns later.

## Proven

- NeoPGP runs on the JavaCard
- secp256k1 key generation works on-card
- the private key remains on-card
- GnuPG recognizes the card as OpenPGP 3.4
- GnuPG can build an OpenPGP identity around the card key
- hardware OpenPGP signing works
- the same public point derives an Ethereum address
- Thurin identity-kit parses the resulting PGP certificate
- the card can PGP-sign its derived Ethereum address
- Thurin can verify that attestation with secp256k1 verification enabled
- SAMA5D3 Buildroot detects the OMNIKEY reader through PC/SC
- embedded Linux can select the NeoPGP applet
- embedded Linux can read the correct signing-key fingerprint
- embedded Linux confirms ECDSA / secp256k1 signing attributes
- embedded Linux can request a real hardware signature
- the resulting signature independently verifies against the known public key
- experimental Keycard Shell NeoPGP detection firmware builds successfully

## Not Yet Proven

- custom NeoPGP firmware running on retail Keycard Shell
- OpenPGP signing through Shell itself
- trusted-display approval
- physical approve / reject controls
- structured signing-intent format
- OpenPGP animated QR request/response
- GnuPG asynchronous virtual-card bridge
- Thurin CLI hardware-signer integration
- production backup/recovery
- safe production use of one key across PGP and Ethereum

## Next Milestone: Trusted Display + Physical Approval

The cryptographic path on the embedded Linux reference platform is working.

The next target is:

```text
structured signing request
        ↓
embedded Linux device
        ↓
display human-readable intent
        ↓
physical approve / reject
        ↓
NeoPGP JavaCard
        ↓
hardware signature
```

After that:

1. add QR request decoding
2. add QR signature response
3. make the host workflow asynchronous
4. complete an air-gapped PGP ↔ Ethereum identity-binding demo
5. port the proven interaction model onto Keycard Shell

The larger research question remains:

> What should OpenPGP hardware interaction look like if designed around secure elements, trusted displays, structured intent, and asynchronous air-gapped QR transport instead of assuming a permanently connected smart-card reader?

## Security

Experimental research only.

Do not use this prototype for meaningful funds, production identities, or sensitive long-lived keys.

Using one private key across OpenPGP and Ethereum collapses security domains. A production design needs strict protocol separation, trusted display, and explicit physical authorization.

No private keys, PINs, local GnuPG state, firmware signing keys, or other sensitive development credentials should be committed to this repository.

## Documentation

- [End-to-End QR Signing Prototype](docs/END_TO_END_QR_SIGNING.md) — optical signing requests, trusted display review, physical approve/reject gating, NeoPGP hardware signing, and QR responses.
