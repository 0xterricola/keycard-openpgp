# Keycard applet: OpenPGP feasibility findings

Source reviewed: `keycard-tech/status-keycard` @ `6f8544a`

## Question

Can the existing Keycard applet serve as the signing backend for OpenPGP,
with all PGP-specific logic implemented on Shell, avoiding an applet install?

## Summary

Yes for the core signing and key-export primitives. No applet change is
required to produce OpenPGP signatures. Two findings need a decision:
curve coverage, and the absence of a signature counter.

## SIGN

`KeycardApplet.java`

    INS_SIGN = 0xC0

- Input is a bare precomputed hash. For ECDSA the length is
  `MessageDigest.LENGTH_SHA_256` (1059), i.e. exactly 32 bytes.
- `secp256k1.signHash(...)` is called directly (1084); the card does not
  rehash or wrap the input.
- Requires an open secure channel and either a verified PIN or a
  pinless key (1037).

This is exactly the primitive OpenPGP needs: the host builds the
certification preimage, hashes it, and submits 32 bytes.

## Signature encoding

`SECP256k1.java`

    ecdsaSign -> crypto.ecdsa.signPreComputedHash(...)  (195)
              -> crypto.fixS(...)                        (196)

- Output is DER-encoded ECDSA (SEQUENCE of two INTEGERs).
- `fixS` applies low-S normalisation (BIP-62). Valid ECDSA; harmless for
  OpenPGP, but the card will never emit a high-S signature.
- To build an OpenPGP signature packet: parse DER for r and s, re-encode
  as MPIs.

## Response structure

The SIGN response is a TLV template containing the public key and the
signature together:

    TLV_SIGNATURE_TEMPLATE (0xA0)
      TLV_PUB_KEY  <KEY_PUB_SIZE bytes, uncompressed point>
      <DER ECDSA signature>

Convenient: the public point needed for fingerprint derivation arrives
with the signature.

## EXPORT KEY

    INS_EXPORT_KEY = 0xC2
    EXPORT_KEY_P2_PUBLIC_ONLY      = 0x01
    EXPORT_KEY_P2_EXTENDED_PUBLIC  = 0x02

- Public-only export exists, addressable by BIP-32 derivation path.
- Requires an open secure channel and a verified PIN (1093).

Sufficient to derive a PGP fingerprint and an Ethereum address from the
same point.

## Curve coverage

`SECP256k1.signHash` dispatches on algorithm:

    SIGN_ECDSA          implemented
    SIGN_BIP340_SCHNORR implemented
    SIGN_ED25519        throws SW_FUNC_NOT_SUPPORTED
    SIGN_BLS12_381      throws SW_FUNC_NOT_SUPPORTED

secp256k1 only in practice. Ed25519 is present in the dispatch but
unimplemented.

Implication: users cannot bring an existing OpenPGP identity. Most
modern PGP keys are Ed25519 or RSA. Any key used here must be newly
generated on secp256k1, which GnuPG and OpenPGP.js both reject by
default today.

Open question for the Keycard team: is Ed25519 planned, or deliberately
excluded?

## No signature counter

Searched the applet for any monotonic signing counter. The only counters
present are:

- `SecureChannelV2.nonceCounter` — AES-CCM nonce, transient
- PIN / PUK remaining-tries counters

There is no count of signing operations.

This is the one genuine regression versus NeoPGP. OpenPGP cards maintain
a signature counter by specification. The current SAMA5D3 demo uses it as
hardware-side evidence that the REJECT path never invoked the key: the
counter holds at N on reject and moves to N+1 on approve. That evidence
comes from the card itself rather than from the application's own logs.

On the Keycard applet this property is unavailable. Absence of output on
the reject path is weaker: it shows nothing was returned, not that the
key was never exercised.

Open question for the Keycard team: would a monotonic counter incremented
on SIGN be considered? It is a small addition compared to shipping an
OpenPGP applet, and it is what makes the approval guarantee externally
verifiable.

## Conclusion

The applet-free path is viable. The signing and export primitives are
already correct for OpenPGP. What remains is a product decision on curve
coverage, and a question about whether hardware-side proof of
non-signing is worth a small applet change.

## Hardware check: retail Keycard in the field

Read-only SELECT against a retail Keycard (identifiers withheld):

    applet version   3.1
    initialized      true
    capabilities     SECURE_CHANNEL | KEY_MANAGEMENT |
                     CREDENTIALS_MANAGEMENT | NDEF

The applet source reviewed above is newer than what is on this card.
Several current features are gated on applet >= 4.0 (e.g. BIP85 export),
and Secure Channel V2 postdates this version.

Implication for "ship to all existing holders": cards in the field are
not necessarily on the current applet. Shell-side PGP logic would need to
target the older command set, or adoption depends on holders updating.

Open question for the Keycard team: can the applet be updated in place on
an initialized card, or does it require a reinstall that clears keys?

## Applet updates are not possible on retail cards

Retail Keycards ship with randomised ISD keys. The official build guide
notes that retaining those keys is what preserves the ability to reinstall
or update the applet; retail holders do not have them. Applet install
instructions are explicitly scoped to development cards.

Consequence: the 3.1 card tested above cannot be moved to a newer applet.
It stays 3.1 for its lifetime.

## Applet 4.0 splits the field (needs confirmation)

Per a third-party project tracking Keycard SDK changes (keycard-pal
issue #304), applet 4.0 was tagged 2026-09-14 and:

- replaces the pairing-based secure channel with a pairing-less,
  certificate-authenticated one
- removes PAIR, UNPAIR, MUTUALLY AUTHENTICATE, IDENTIFY CARD
- changes the shape of the SELECT response
- Secure Channel V2 uses ephemeral ECDH and AES-CCM, with the card
  authenticating itself by signing the handshake transcript against a
  CA-certified key

Because locked cards are immutable, 3.x cards in the field remain 3.x
indefinitely. Any client must speak both protocols for as long as both
generations exist.

NOT YET CONFIRMED with the Keycard team. Source is a downstream project's
issue tracker, not official documentation.

Implication for "ship PGP to all existing Keycard holders": the installed
base is permanently split across two incompatible secure-channel
protocols. Shell-side PGP logic would need to be dual-stack, or target
one generation and exclude the other.

Also noted: on 4.0, self-flashed development cards whose certificate does
not chain to the known CA are rejected at SELECT by the standard SDK.
Relevant to any plan that involves a self-built research card.

## Self-flashing a research card: 4.0 fails, 3.2 works

Target: blank NXP JavaCard (ex-PhononDAO), GlobalPlatform default ISD keys,
OP_READY, installed with GlobalPlatformPro over PC/SC.

Applet 4.0:

- the release cap bundles four applets; `--install` requires naming one
- it imports `A0000008040002` (`im.status.keycard.math`), which is NOT
  shipped with the release. It comes from the `keycard-math` git
  submodule, so the repo must be cloned with `--recurse-submodules`.
  Without it, LOAD fails with 0x6438 (imported package not available)
- with the math package loaded first via `--load`, the Keycard package
  loads cleanly, but instantiating `A000000804000101` fails with
  0x6985 (conditions of use not satisfied)
- `IdentApplet` (`A000000804000104`) from the same package instantiates
  without error on the same card in the same session

Applet 3.2:

- no `keycard-math` import
- installs and instantiates on the same card without issue
- SELECT returns the uninitialized shape: version 0.0, null instance and
  key UID, capabilities SECURE_CHANNEL | CREDENTIALS_MANAGEMENT

Reading: the 4.0 failure is isolated to the Keycard applet's constructor,
not to loading, privileges or install parameters. The README requires
JavaCard 3.0.5 and names `KeyAgreement.ALG_EC_SVDP_DH_PLAIN_XY` as the
3.0.5-specific requirement; Secure Channel V2 uses ECDHE on secp256k1.
This card generation runs applets targeting 3.0.4 (it previously ran
NeoPGP) but appears to lack what 4.0 needs.

Consequences:

- a JavaCard generation sufficient for an OpenPGP applet is not
  necessarily sufficient for current Keycard
- the self-flashing path for 4.0 has an undocumented prerequisite (the
  math package from a submodule). Shipping it alongside the release cap,
  or noting it in the release, would save others the 0x6438
