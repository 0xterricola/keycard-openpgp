# Keycard applet: OpenPGP feasibility findings

## Question

Can the existing Keycard applet serve as the signing backend for OpenPGP,
with all PGP-specific logic implemented on the host or on Shell, avoiding
an applet install?

## Answer

Yes. Demonstrated end to end on a retail Keycard with no applet changes
of any kind.

Hardware: retail Keycard, applet 3.1. Host: macOS, OMNIKEY reader,
Python `keycard` SDK 0.3.0 over PC/SC.

1. SELECT, open Secure Channel V1 with an existing pairing, VERIFY PIN
2. EXPORT KEY (public only) at `m/44'/60'/0'/0/0` -> 65-byte uncompressed
   SEC1 point
3. build a v4 public-key packet body from that point with a fixed
   creation time
4. derive the fingerprint: `A1265C689EC7C60018C0AFB023A92D87D63F7E7C`
5. build the certification preimage over key body + UID `keycard-test`
6. SIGN the 32-byte digest on the card
7. assemble public-key + UID + signature packets
8. import into a scratch GnuPG keyring

Result:

    pub   secp256k1 [SCA]
          A1265C689EC7C60018C0AFB023A92D87D63F7E7C
    uid           keycard-test
    sig!3        23A92D87D63F7E7C  [self-signature]

    gpg: 1 good signature

`sig!3` means GnuPG verified the signature cryptographically. The
self-certification was produced by the card.

Notes:

- EXPORT KEY and SIGN both used `make_current=False`; no persistent card
  state was changed
- the creation time is part of the fingerprint preimage. It must be a
  deliberate, fixed choice: changing it changes the key's identity
- the signature result also returns the public point, matching EXPORT KEY

## Why it works

Source reviewed: `keycard-tech/status-keycard` @ `6f8544a`

### SIGN

`KeycardApplet.java`, `INS_SIGN = 0xC0`

- Input is a bare precomputed hash. For ECDSA the length is
  `MessageDigest.LENGTH_SHA_256` (1059), i.e. exactly 32 bytes.
- `secp256k1.signHash(...)` is called directly (1084); the card does not
  rehash or wrap the input.
- Requires an open secure channel and either a verified PIN or a pinless
  key (1037).

Exactly the primitive OpenPGP needs: the host builds the certification
preimage, hashes it, and submits 32 bytes.

### EXPORT KEY

    INS_EXPORT_KEY = 0xC2
    EXPORT_KEY_P2_PUBLIC_ONLY      = 0x01
    EXPORT_KEY_P2_EXTENDED_PUBLIC  = 0x02

Public-only export, addressable by BIP-32 path. Requires an open secure
channel and a verified PIN (1093). Sufficient to derive a PGP fingerprint
and an Ethereum address from the same point.

### Signature encoding

`SECP256k1.java`

    ecdsaSign -> crypto.ecdsa.signPreComputedHash(...)  (195)
              -> crypto.fixS(...)                        (196)

The applet emits DER-encoded ECDSA (SEQUENCE of two INTEGERs), wrapped in
a TLV template alongside the public point:

    TLV_SIGNATURE_TEMPLATE (0xA0)
      TLV_PUB_KEY  <KEY_PUB_SIZE bytes, uncompressed point>
      <DER ECDSA signature>

`fixS` applies low-S normalisation (BIP-62). Valid ECDSA; harmless for
OpenPGP, but the card will never emit a high-S signature.

In practice the Python SDK parses the TLV and returns `r` and `s` already
split, so no DER decoding is needed on that path. A C helper
(`ecdsa_der.c`) is included for clients that talk APDUs directly.

## Open questions for the Keycard team

### Is Ed25519 planned?

`SECP256k1.signHash` dispatches on algorithm:

    SIGN_ECDSA          implemented
    SIGN_BIP340_SCHNORR implemented
    SIGN_ED25519        throws SW_FUNC_NOT_SUPPORTED
    SIGN_BLS12_381      throws SW_FUNC_NOT_SUPPORTED

secp256k1 only in practice; Ed25519 is present in the dispatch but
unimplemented.

This is the largest practical limitation. Users cannot bring an existing
OpenPGP identity — most modern PGP keys are Ed25519 or RSA. Any key used
here must be newly generated on secp256k1, which GnuPG and OpenPGP.js
both reject by default today.

### Would a monotonic SIGN counter be considered?

The applet has no count of signing operations. The only counters present
are `SecureChannelV2.nonceCounter` (AES-CCM nonce, transient) and the
PIN / PUK remaining-tries counters.

OpenPGP cards maintain a signature counter by specification. The SAMA5D3
trusted-display prototype uses it as hardware-side evidence that a
rejected request never invoked the key: the counter holds at N on reject
and moves to N+1 on approve. That evidence comes from the card itself
rather than from the application's own logs.

On the Keycard applet the property is unavailable. Absence of output on
the reject path is weaker — it shows nothing was returned, not that the
key was never exercised.

A monotonic counter incremented on SIGN is a small addition compared to
shipping an OpenPGP applet, and it is what makes the approval guarantee
externally verifiable.

### Which cards are used for 4.0 development?

See the appendix: a self-flashed 4.0 card was not achievable on the
JavaCard generation tested here.

## Importing an existing PGP key is not possible

`LOAD KEY` (`INS_LOAD_KEY = 0xD0`) accepts four forms: EC keypair,
extended EC keypair, BIP-32 seed, LEE seed.

`loadKeyPair` writes `masterPrivate.setS(...)` and `masterPublic.setW(...)`
inside a transaction, then regenerates the key UID from `masterPublic`.
There is one master key on the card; loading replaces it. When no public
key is supplied, the applet derives one with `secp256k1.derivePublicKey`,
so the loaded key is secp256k1 regardless.

Consequences:

- a user cannot bring an existing OpenPGP key onto a Keycard. Loading one
  would overwrite the wallet master and destroy the wallet keys.
- Shell cannot work around this. The applet has a single master key slot;
  there is nothing to partition.
- Ed25519 support alone would not change this. The obstacle is the key
  model, not only the curve.

The viable path is derivation, not import: a PGP identity derived from
the card's existing master at a chosen BIP-32 path. This is what the
end-to-end proof above does. It coexists with the wallet, requires no
applet change, and is reproducible from the seed, which also gives a
recovery story.

Implication for the product story: not "use your existing PGP key on a
Keycard" but "derive a hardware-backed PGP identity from your Keycard".
That is a new key with a new fingerprint, which a user would need to
cross-sign from an existing identity to carry over any trust.

## Two backends, one interface

The constraints above argue for supporting two card backends rather than
choosing between them.

**Keycard applet.** Works on cards people already own, no install, no
applet change. Identity is derived from the existing master, so the
wallet is untouched. Limited to secp256k1, and there is no signature
counter.

**An OpenPGP applet on a user-supplied card.** Full OpenPGP semantics,
including the signature counter and whatever curves the applet supports.
Costs the user a card and a flashing step.

These are not exclusive. SELECT identifies which applet is present before
any secure channel, PIN or pairing, so routing costs nothing and happens
on the first APDU of a tap.

The existing code is already shaped for this. `openpgp_v4.c` consumes and
produces byte arrays with no knowledge of the card; the card dependency
is confined to a four-function interface (`neopgp_sign.h`). The Keycard
path demonstrated above is a second implementation behind that same
boundary.

Costs to weigh:

- two secure-channel implementations in firmware (Keycard's pairing-based
  channel, plus whatever the OpenPGP applet requires)
- two sets of failure modes to surface on a constrained UI

Open design question: the two backends do not offer the same guarantees.
One can prove from the card that a rejected request never invoked the
key; the other cannot. A guarantee that silently varies by card is the
same two-tier problem this project exists to avoid, so the trusted
display should probably state which mode it is operating in.

---

## Supporting validation

`openpgp_v4_build_public_key_body` was checked against a known-good key
(the NeoPGP secp256k1 signing key, fingerprint `31CE69D6...`). The
constructed body matches `gpg --export` byte for byte, and the derived
fingerprint matches.

DER-to-raw ECDSA conversion is covered by 14 tests including high-bit
leading zeros, short values needing left-padding, and malformed input.

# Appendix: the installed base is split

## Retail cards cannot be updated

Read-only SELECT against a retail Keycard (identifiers withheld):

    applet version   3.1
    initialized      true
    capabilities     SECURE_CHANNEL | KEY_MANAGEMENT |
                     CREDENTIALS_MANAGEMENT | NDEF

Retail Keycards ship with randomised ISD keys. The official build guide
notes that retaining those keys is what preserves the ability to
reinstall or update the applet; retail holders do not have them. Applet
install instructions are explicitly scoped to development cards.

Consequence: this card stays on 3.1 for its lifetime.

Incidental observation: this card's pairing secret was still the
published default (`KeycardDefaultPairing`). Pairing alone does not
expose keys — PIN verification is still required for SIGN and EXPORT KEY
— but it is worth knowing that cards in the field may not have had it
changed.

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

NOT CONFIRMED with the Keycard team. Source is a downstream project's
issue tracker, not official documentation.

Implication for "ship PGP to all existing Keycard holders": the installed
base is permanently split across two incompatible secure-channel
protocols. Shell-side PGP logic would need to be dual-stack, or target
one generation and exclude the other.

Also noted: on 4.0, self-flashed development cards whose certificate does
not chain to the known CA are reportedly rejected at SELECT by the
standard SDK.

## Self-flashing a research card: 4.0 fails, 3.2 works

Target: blank NXP JavaCard from a PhononDAO batch, GlobalPlatform default
ISD keys, OP_READY, JavaCard 3.0.4 per CPLC. Installed with
GlobalPlatformPro over PC/SC.

This card has never held another applet. A sibling card from the same
batch runs NeoPGP; both report an identical ATR
(`3b:dc:18:ff:81:91:fe:1f:c3:80:73:c8:21:13:66:05:03:63:51:00:02:50`).

Applet 4.0:

- the release cap bundles four applets; `--install` requires naming one
- it imports `A0000008040002` (`im.status.keycard.math`), which is NOT
  shipped with the release. It comes from the `keycard-math` git
  submodule, so the repo must be cloned with `--recurse-submodules`.
  Without it, LOAD fails with `0x6438` (imported package not available)
- with the math package loaded first via `--load`, the Keycard package
  loads cleanly, but instantiating `A000000804000101` fails with `0x6985`
  (conditions of use not satisfied)
- `IdentApplet` (`A000000804000104`) from the same package instantiates
  without error on the same card in the same session

Applet 3.2:

- no `keycard-math` import
- installs and instantiates on the same card without issue
- SELECT returns the uninitialized shape: version 0.0, null instance and
  key UID, capabilities `SECURE_CHANNEL | CREDENTIALS_MANAGEMENT`

Reading: the 4.0 failure is isolated to the Keycard applet's constructor,
not to loading, privileges or install parameters. The README requires
JavaCard 3.0.5 and names `KeyAgreement.ALG_EC_SVDP_DH_PLAIN_XY` as the
3.0.5-specific requirement; Secure Channel V2 uses ECDHE on secp256k1.
That algorithm is reported as supported by only a small minority of cards
in the public JCAlgTest database. This card generation appears to lack
it, but that has not been confirmed directly.

Consequences:

- a JavaCard generation sufficient for an OpenPGP applet is not
  necessarily sufficient for current Keycard
- the self-flashing path for 4.0 has an undocumented prerequisite (the
  math package from a submodule). Shipping it alongside the release cap,
  or noting it in the release, would save others the `0x6438`

