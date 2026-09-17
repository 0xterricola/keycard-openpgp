# keycard-openpgp

Experimental OpenPGP work on programmable JavaCards, targeting Keycard Shell as an air-gapped hardware interface.

## Thesis

Can a removable JavaCard hold a hardware-backed OpenPGP identity while the same `secp256k1` public key also deterministically defines an Ethereum address?

Longer-term:

```text
GnuPG / Thurin CLI
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

## Current Status

Core cryptographic proof-of-concept works end-to-end.

### Hardware

- HID Global OMNIKEY 3x21 reader
- NXP JavaCard originally used in the PhononDAO alpha
- GlobalPlatform 2.1.1 / SCP02
- JavaCard 3.0.4 compatible
- Original Phonon applet removed
- NeoPGP installed

Historical lineage:

```text
Status / Keycard
      ↓
GridPlus
      ↓
Phonon
      ↓
this experiment
```

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
0x9ce2e20fc392304fd1e50541ec67168913b5f3ff
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

## Thurin Compatibility

`@thurinlabs/identity-kit` successfully parses the certificate:

```text
fingerprint:
31CE69D66A5E9DE0F977B59C79BB391497E8E6D4

userIDs:
terricola-testtt

algorithm:
secp256k1
```

Two OpenPGP.js compatibility details were found.

`eckey-utils` is required at runtime for secp256k1 self-certification verification:

```bash
npm install eckey-utils
```

OpenPGP.js rejects secp256k1 message verification by default. Verification succeeds with:

```ts
config: {
  rejectCurves: new Set(),
}
```

With that policy enabled in Thurin's `verifyAttestation()`, the complete hardware-signed attestation returns:

```text
{ verified: true }
```

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

## Not Yet Proven

- Keycard Shell support for NeoPGP
- OpenPGP APDU handling in Shell firmware
- trusted-display approval
- OpenPGP animated QR request/response
- GnuPG asynchronous virtual-card bridge
- Thurin CLI hardware-signer integration
- production backup/recovery
- safe production use of one key across PGP and Ethereum

## Next Milestone: Keycard Shell

First target:

```text
insert NeoPGP JavaCard
        ↓
Shell selects OpenPGP AID
        ↓
Shell reads metadata
        ↓
display:

OPENPGP CARD
secp256k1
31CE69D6...E6D4
```

Then:

1. NeoPGP APDU signing from Shell
2. trusted-display approval
3. animated QR signing request
4. QR signature response
5. GnuPG / Thurin CLI helper

The larger research question:

> What should OpenPGP hardware interaction look like if designed around secure elements, trusted displays, and asynchronous air-gapped QR transport instead of assuming a permanently connected smart-card reader?

## Security

Experimental research only.

Do not use this prototype for meaningful funds, production identities, or sensitive long-lived keys.

Using one private key across OpenPGP and Ethereum collapses security domains. A production design needs strict protocol separation, trusted display, and explicit physical authorization.
