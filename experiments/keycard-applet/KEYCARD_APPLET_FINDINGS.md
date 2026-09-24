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
