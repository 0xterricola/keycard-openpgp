# Keycard as an OpenPGP backend: summary for the Keycard team

A short version of `KEYCARD_APPLET_FINDINGS.md`, covering only what needs
a decision.

## In short

- **Can the existing applet back OpenPGP with no applet changes?** Yes.
  Proven end to end on a retail card at applet 3.1, verified by stock GnuPG.
- **Can users bring an existing PGP key?** No, and this cannot be worked
  around. One master key slot; loading a key destroys the wallet.
  Derivation from the existing master is the only path.
- **What is lost versus an OpenPGP card?** The signature counter. The
  approval guarantee becomes an assertion rather than something the card
  demonstrates.
- **One ask:** a monotonic counter incremented on SIGN.
- **One question, and it is load-bearing:** is Ed25519 planned? Without
  a curve the ecosystem already accepts, the identities this produces
  are verifiable only by people who reconfigure their tooling.
- **One decision:** whether Shell routes on applet type, supporting both the
  Keycard applet and an OpenPGP applet on a user-supplied card.

## It works, with no applet changes

Exported a public key from a retail Keycard at applet 3.1, derived at
`m/44'/60'/0'/0/0`, built an OpenPGP identity around the point, signed the
self-certification on the card, and stock GnuPG verifies it:

    pub   secp256k1 [SCA]
          A1265C689EC7C60018C0AFB023A92D87D63F7E7C
    uid           keycard-test
    sig!3        23A92D87D63F7E7C  [self-signature]

    gpg: 1 good signature

`sig!3` means GnuPG checked the signature cryptographically. SIGN taking a
bare 32-byte hash is exactly the primitive OpenPGP needs.

## Import is off the table

`loadKeyPair` writes to the single master key slot and regenerates the key
UID from it. Loading a PGP key would overwrite the wallet master and
destroy the wallet keys. Shell cannot work around this — there is one slot,
nothing to partition — and Ed25519 support would not change it, because the
obstacle is the key model rather than the curve.

So derivation is the only path: a PGP identity derived from the card's
existing master at a fixed BIP-32 path. It coexists with the wallet and is
reproducible from the seed.

**Recommendation:** standardise the path, the way EIP-1581 does for
non-wallet keys. Pick it once and the identity becomes reproducible across
implementations.

## No signature counter

The applet has no count of signing operations. The only counters present are
the Secure Channel nonce and the PIN/PUK retry counters.

The SAMA5D3 trusted-display prototype uses an OpenPGP card's signature
counter as its main proof: present the same request twice, reject once,
approve once, and the counter holds then moves. That evidence comes from the
card rather than from the application's own logs. On Keycard the property is
unavailable, and the approval guarantee becomes something the software
asserts rather than something the card demonstrates.

**Recommendation, and the one ask here:** a monotonic counter incremented on
SIGN. Small next to shipping an OpenPGP applet, and it makes an approval
guarantee checkable from outside — useful for any Shell flow, not only PGP.

## Ed25519

`signHash` has `SIGN_ED25519` in the dispatch but it throws
`SW_FUNC_NOT_SUPPORTED`.

This matters more than it first appears. Derivation means new keys
regardless, so the curve is not a migration question — it is an acceptance
question. `gpg-card` refuses secp256k1 from its allowlist; OpenPGP.js
rejects it unless `rejectCurves` is cleared. Neither is an oversight.
secp256k1 has no standing in the OpenPGP specifications, and those
rejections are considered positions.

The consequence is an identity that is cryptographically sound but
socially awkward: verifiable by anyone who reconfigures their tooling,
invisible to everyone who does not. For a key whose entire value comes
from other people trusting it, that is a weak foundation.

Two ways this resolves. Either the applet supports a curve the ecosystem
already accepts, or the identity draws its legitimacy from somewhere other
than PGP convention — an Ethereum binding, an attestation layer. The
second is possible but means building trust infrastructure rather than
inheriting it.

Everything demonstrated here is curve-agnostic. The same code path works
unchanged the day Ed25519 lands.

**Question:** is Ed25519 planned, or deliberately out of scope?

## Two backends need not be exclusive

The Keycard applet and an OpenPGP applet on a user-supplied card can both be
supported. SELECT identifies which is present before any secure channel, PIN
or pairing, so routing costs nothing. The existing PGP code is already
card-agnostic behind a four-function interface.

- **Keycard applet** — works on cards people already own, no install,
  identity derived from the existing master. secp256k1 only, no counter.
- **OpenPGP applet on a user-supplied card** — full OpenPGP semantics
  including the counter. Costs the user a card and a flashing step.

**Open design question:** the two do not offer the same guarantees. One can
prove from the card that a rejected request never invoked the key; the other
cannot. A guarantee that varies silently by card is the two-tier problem this
project exists to avoid, so the trusted display should probably state which
mode it is in.

## There is no QR convention for OpenPGP operations

BC-UR covers PSBTs and Ethereum transactions. Keyserver QR codes carry a
fingerprint or a URL. Nothing covers carrying a certification request to an
air-gapped signer and bringing the signature back.

The prototype uses an ad-hoc framing. Before building more on it, the
payloads are worth defining properly — either as BC-UR types, or one layer up
as a generic signed-intent format where OpenPGP is one intent type among
several.

The second is the more useful shape: it is what makes Shell a general-purpose
air-gapped signing interface rather than a wallet with a PGP feature.

## Full detail

`KEYCARD_APPLET_FINDINGS.md`, plus the scripts and C helpers in this
directory.
