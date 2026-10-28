# keycard-openpgp

**OpenPGP signing research, protocol documentation, and reference tooling for Keycard Shell.**

> **Upstream milestone — October 14, 2026:** The Keycard Shell OpenPGP work
> was merged into [`keycard-tech/keycard-shell` (PR #227)](https://github.com/keycard-tech/keycard-shell/pull/227).
> Merging into the upstream repository is not the same as an official firmware
> release; check Keycard Shell release notes before assuming a published build
> contains these changes.

## Keycard Shell OpenPGP status

The upstream firmware implementation supports two operations on physical
Keycard Shell hardware:

- **`CREATE_IDENTITY`** — derives the OpenPGP identity using Shell-controlled
  key-selection policy, presents the UID, creation time, and fingerprint for
  on-device review, then generates a self-certified OpenPGP v4 public certificate
  using a Keycard-backed secp256k1 key after approval.
- **`SIGN_MESSAGE`** — validates and displays reviewable message text, requires
  physical approval, canonicalizes text for OpenPGP signing, and returns an
  OpenPGP v4 detached signature. The firmware's documented review limit is
  **255 message bytes**; some experimental host tooling has a lower limit.

Requests and responses use the **`UR:BYTES` QR transport** via
`Extras → OpenPGP` on Shell. Host applications construct requests and verify
returned artifacts; private-key operations and authorization remain on Shell
and the Keycard. The firmware implementation and hardware validation are
recorded in [upstream PR #227](https://github.com/keycard-tech/keycard-shell/pull/227).

This repository holds supporting research and developer tools, including:

- [The standalone Shell OpenPGP reference client](reference-client/) for QR
  request generation and response decoding ([PR #43](https://github.com/0xterricola/keycard-openpgp/pull/43)).
- [The version 1 protocol documentation](https://github.com/0xterricola/keycard-openpgp/pull/42),
  covering CBOR requests, QR framing, firmware review rules, and response formats.
- [Hardware-backed interoperability verification](https://github.com/0xterricola/keycard-openpgp/pull/41)
  using GnuPG, RNP/Botan, and OpenPGP.js under documented compatibility constraints.
- [GapPGP integration overview](docs/GAPPGP.md) — architecture, security model, and interoperability.
- [GapPGP for Thunderbird](https://github.com/0xterricola/gappgp-thunderbird),
  a separate experimental host integration that demonstrates hardware-backed
  signed MIME messages. Native WebExtension camera acquisition remains blocked
  in the tested Thunderbird builds
  ([Mozilla bug #2080573](https://bugzilla.mozilla.org/show_bug.cgi?id=2080573));
  external QR capture and manual response transfer have been demonstrated.

**Security and release scope:** This is experimental research and integration
software, not a security audit or a production assurance claim. The merged
firmware functionality has been tested on physical hardware, but individual
host integrations and future official releases have their own validation and
compatibility requirements. Do not use experimental host tools for sensitive
long-lived keys or meaningful funds without independently assessing the risks.

## Historical reference-device research

The material below documents earlier **SAMA5D3 + NeoPGP research**, including
an experimental `KC1` QR envelope and a separate OpenPGP packet path. These
are useful archived engineering experiments, **not** the current Keycard Shell
`UR:BYTES` protocol or the merged firmware implementation.

In that research setup, an application supplied structured intent to a trusted
reference device. The SAMA5D3 used a display, keypad, camera, and programmable
research JavaCard running NeoPGP; this did not replace the official Keycard
applet. The recorded embedded artifact produced a GnuPG-accepted signature and
an attestation verified by `identity-kit`. Integrating those separate SAMA5D3
paths into one general QR-to-OpenPGP signing workflow was not completed; the
upstream Keycard Shell work is an independent integration milestone.

See also the historical [roadmap](docs/ROADMAP_STATUS.md). Dated findings below
should be read as research checkpoints, not current Shell feature limitations.

## SAMA5D3 reference architecture (historical)

| Layer | Current implementation | Direction |
| --- | --- | --- |
| Trusted display, controls, PIN, camera | SAMA5D3 + ST7789 + USB camera + 3×4 keypad | Keycard Shell or another Shell-class device |
| Trusted protocol adapters | OpenPGP v4 digest/packet path; separate raw-signature QR demo | Integrated OpenPGP interface, then Ethereum/EVM |
| Request/response framing | Temporary `KC1` envelope | Protocol-appropriate structured requests and standard results |
| Transport | Sequential QR request and response | Study Shell's UR/CBOR QR machinery and multipart transport |
| Secure element (SE) | NeoPGP on programmable research JavaCard | Official Keycard applet is a preferred target; the architecture is not tied to one applet |

```text
Application / host connector
    builds structured request; consumes and verifies result
                    |
         request/response framing
                    |
               QR transport
                    |
    +------- TRUSTED DEVICE BOUNDARY -------+
    |                                      |
    |  Trusted protocol adapter            |
    |    parse and validate structured data|
    |    derive protocol bytes/digest      |
    |    supply the same meaning to review |
    |                  |                   |
    |  Trusted presentation / policy       |
    |                  |                   |
    |  Physical APPROVE / REJECT            |
    |                  |                   |
    |  Local PIN and card interface ---------> Removable SE
    |                               <-------- private-key operation
    |  Trusted adapter constructs result   |
    +------------------|-------------------+
                       |
             response transport
                       |
             application / host
```

Protocol adapters are separate from the generic signer core, which manages
the request lifecycle, approval, PIN handling, and card access. **Both run
inside the trusted-device boundary wherever their logic is needed to bind
human review to the signature.** Application workflows, broadcasting, and
registry access remain outside that boundary.

`KC1` is framing, QR is transport, OpenPGP and EIP-712 define signing semantics,
and Thurin is an application. These are distinct layers.

## Core security rule

> The human-readable display must be cryptographically bound to the exact
> bytes or digest authorized for signing. The trusted device must independently
> derive those bytes or that digest from the validated structured request.

The online host must not be able to pair friendly display text with an
unrelated opaque digest. Trusted parsing, protocol-specific encoding and
hashing, and review must describe the same operation. Unsupported or
unrenderable requests must be rejected rather than truncated, guessed, or
signed with only an opaque hash shown.

Request rejection, PIN cancellation, or failed PIN verification must prevent
signing. PIN digits stay local to the trusted device, with masked progress;
they must never be logged or returned over the transport. The private key
stays in the secure element.

An internal digest-signing primitive such as `neopgp_sign_digest()` is useful
below the trusted protocol adapter. It must not become an unrestricted
host-facing `SIGN_HASH` interface. The host may package returned results, but
cannot choose signing bytes or metadata affecting the signed meaning unless
the trusted adapter has independently validated them and included them in the
protocol-defined digest.

These were requirements for the SAMA5D3 research setup. Its incomplete
rendering and integration work must not be conflated with Keycard Shell.

## Demonstrated status and remaining gaps (historical research)

The SAMA5D3 entries below are historical. The separate upstream Shell
implementation is summarized at the top of this README.


| Path | Recorded evidence | Scope |
| --- | --- | --- |
| Connected GnuPG / research card | OpenPGP 3.4 recognition, on-card secp256k1 key, normal GnuPG signatures | Connected smart-card interoperability |
| SAMA5D3 `KC1` QR loop | Camera input, trusted review of demonstrated messages, physical approval/rejection, local keypad PIN, raw signature response | Locally hashes the message with SHA-256; returns raw ECDSA |
| Direct embedded OpenPGP path | Device constructs v4 digest and Signature Packet; NeoPGP signs; GnuPG reports `Good signature` | Fixed-message hardware test, separate from QR review |
| Canonical Thurin OpenPGP artifact | Tracked clear-signed artifact returns `{ verified: true }` with `@thurinlabs/identity-kit` 1.1.1 | OpenPGP attestation verification, not Ethereum authorization |
| Keycard Shell | Physical hardware-tested `CREATE_IDENTITY` and `SIGN_MESSAGE` flows using Keycard-backed signing and QR | [Merged upstream in PR #227](https://github.com/keycard-tech/keycard-shell/pull/227); not an official firmware release |

The historical SAMA5D3 interaction tests used the NeoPGP signature counter as hardware evidence:

- Approved request and successful local PIN signing: `10 → 11`.
- PIN-stage cancellation: `11 → 11`.
- Request rejection: counter unchanged.

The historical SAMA5D3 signer used local keypad PIN entry and masked display feedback;
the terminal PIN dependency has been removed. SSH and Ethernet still serve
development and launch/debugging needs, so the board is not yet demonstrated
as a fully network-disconnected device.

Not yet demonstrated or completed **in the SAMA5D3 research prototype**
(not limitations of the merged Shell implementation):

- A single structured OpenPGP QR request → trusted review → standard artifact
  response flow, including general text handling and complete rendering.
- A hardware EIP-712 signature through this project's trusted Ethereum
  adapter, or EVM transaction signing through that adapter.
- The full Thurin `--sign-out` → hardware signer → `authorize finish` flow,
  followed by a hardware-authorized Sepolia attestation and registry lookup.
- Generic request correlation, replay protection, persistent pending requests,
  retries, or BC-UR/multipart integration in this prototype.
- Network-free SAMA5D3 boot/runtime or use of the official Keycard
  applet within that research prototype.
- Production backup/recovery or a production policy for cross-protocol keys.

## KC1: historical SAMA5D3 QR contract

The historical KC1 request was:

```text
KC1|OP=PGP_SIGN|MSG=KEYCARD OPENPGP TEST
```

The response is:

```text
KC1|FP=<40-hex-character OpenPGP fingerprint>|SIG=<128 hex characters>
```

| Part | Current behavior |
| --- | --- |
| Request prefix | Requires `KC1|OP=PGP_SIGN|MSG=`; signature responses are not accepted as requests |
| `MSG` | Everything after the prefix is message content; parser accepts 1–160 printable ASCII bytes, then the display application rejects messages longer than 20 characters |
| Review | Shows `OPENPGP SIGN`, `NEOPGP CARD`, an abbreviated fingerprint read from the card, the message, and APPROVE/REJECT controls |
| Signing input | The device calculates `SHA-256(message)` from the same parsed message buffer used for display |
| `FP` | OpenPGP fingerprint read from the inserted card, returned as uppercase hex |
| `SIG` | Raw 64-byte secp256k1 ECDSA `r || s`, returned as uppercase hex |
| Correlation/replay | No request ID, nonce, timestamp, or intended-key selector; response has no explicit request ID or digest; replay protection is not implemented |

Despite the `PGP_SIGN` label, this QR path currently returns a raw signature,
not an OpenPGP Signature Packet. A signature over `SHA-256(message)` cannot
simply be wrapped into a valid OpenPGP signature: OpenPGP requires its own
signed fields and trailer in the digest.

**Known display limitation:** the current font renders uppercase letters,
digits, spaces, `.`, `-`, and `:`. Other printable characters accepted by the
parser, including lowercase letters, render as blanks. The 20-character limit
prevents overlength display, but does not establish faithful rendering for
every accepted request. The renderer and acceptance rules must be reconciled
before claiming the core display/signature property for arbitrary inputs.

The host must retain the intended message and expected public key and verify
the returned signature. The response fingerprint alone supplies neither
request correlation nor replay protection.

Current source: [QR parser](embedded/sama5d3/qr_request.c),
[display/approval loop](embedded/sama5d3/sign_prompt_v4.c), and
[card signing API](embedded/sama5d3/neopgp_sign.c).
The [end-to-end QR report](docs/END_TO_END_QR_SIGNING.md) preserves the earlier
test transcript; its terminal-PIN limitation predates the local keypad work.

## SAMA5D3 OpenPGP artifact milestone (historical)

The connected GnuPG path and the direct embedded path both demonstrate
OpenPGP signing, but use different software around the card.

The direct SAMA5D3 implementation now:

1. Builds OpenPGP v4 signature fields.
2. Derives the protocol-specific SHA-256 digest on the device, including the
   signed data, signature fields, and v4 trailer.
3. Passes that locally derived digest to NeoPGP for the secp256k1 operation.
4. Converts raw ECDSA `r || s` to OpenPGP MPIs and builds a Signature Packet.
5. Produces a detached signature recorded as accepted by GnuPG with
   `Good signature from "terricola-testtt"`.

The canonical statement was also signed through this hardware path:

```text
I control the Ethereum address: 0x9ce2e20fc392304fd1e50541ec67168913b5f3ff
```

The resulting signature was assembled into
[`artifacts/hardware-thurin-attestation.asc`](artifacts/hardware-thurin-attestation.asc).
The [hardware attestation test](experiments/thurin/test-hardware-attestation.ts)
uses the [public certificate](artifacts/terricola-testtt.asc) and verifies the
artifact with `@thurinlabs/identity-kit` 1.1.1:

```text
{ verified: true }
HARDWARE OPENPGP ATTESTATION VERIFIED
```

That completes the standards-compliant embedded OpenPGP artifact milestone.
It does not yet provide a general OpenPGP signing service. The
[hardware test](embedded/sama5d3/openpgp_v4_sign_test.c) uses a fixed message,
identity, and creation time, and does not run the QR message-review workflow.
The [v4 helper](embedded/sama5d3/openpgp_v4.c) is a digest/packet component,
not a general cleartext canonicalizer or clear-signing formatter.

This was the intended next integration for the SAMA5D3 experiment. The
subsequent Keycard Shell firmware work implements its own structured
OpenPGP request, trusted review, and standard artifact response flow.
Any canonicalization or signature metadata affecting the digest still
belongs inside the trusted protocol adapter.

## Keycard Shell and Ethereum/EVM research

Keycard Shell is now a physically tested OpenPGP target, with the
identity-creation and message-signing implementation merged upstream in
[PR #227](https://github.com/keycard-tech/keycard-shell/pull/227).
The earlier NeoPGP AID-detection branch was a separate research step;
it does not describe the merged Keycard-backed signing path. An official
firmware release containing these changes has not been established here.

The [Shell/EIP-712 research](docs/research/KEYCARD_SHELL_EIP712.md) records
source observations from the `feature/openpgp-card` branch at `1ca841c`.
These are research findings, not proof that the corresponding integrations
already work in `keycard-openpgp`.

The inspected Shell Ethereum path separates:

```text
ERC-4527 eth-sign-request over UR/CBOR QR
    → trusted structured-data parsing and local digest derivation
    → trusted generic or specialized review
    → human approval
    → card private-key operation
    → Ethereum signature processing
    → ERC-4527 eth-signature over UR QR
```

EIP-712 typed data is one Ethereum signing operation; transactions and
messages are sibling operations. EIP-712 is not a transport protocol, and
Thurin does not need a new signing protocol of its own.

The observations that guide this project are:

- Shell receives structured EIP-712 data and constructs the digest locally.
  Hashing and trusted review use the parsed request context; approval precedes
  the card signing call.
- Generic typed-data review exists alongside specialized presentations for
  recognized structures such as Permit and SafeTx.
- Shell checks the requested source fingerprint against the card-derived
  identity. Optional request IDs are echoed in the response; correlation
  alone does not establish replay protection.
- Recovery-ID derivation is a useful reference for turning raw ECDSA into a
  65-byte Ethereum signature. NeoPGP compatibility remains to be tested.
- The inspected Shell/applet code does not establish a portable low-s
  guarantee. The intended Ethereum adapter must handle canonicalization and
  verify recovery against the expected public key/address above the raw card
  primitive, inside the trusted-device boundary.

Account discovery is a separate compatibility problem. Shell's standard
`crypto-hdkey` export includes a public key, chain code, and origin metadata.
The fixed NeoPGP key has no HD chain code; inventing one would misrepresent
its capabilities. An empty keypath with a fixed-key source fingerprint is
only a candidate signing mapping, not demonstrated wallet pairing. That
four-byte source fingerprint is distinct from the OpenPGP fingerprint.

Test-vector coverage, fixed-key wallet interoperability, chain-ID consistency,
licensing/reuse, and hardware integration remain research tasks. The research
has not established a cross-check between an outer ERC-4527 chain ID and an
EIP-712 domain's `chainId`.

## Thurin integration

Thurin is the first external application integration. It exercises an OpenPGP
identity proof and an Ethereum EIP-712 authorization as separate operations.
The hardware OpenPGP artifact is verified; the hardware Ethereum authorization
flow is still pending.

The inspected Thurin 0.7.0 interface sends EIP-712 typed-data JSON to an
external signer on stdin and expects a 65-byte recoverable Ethereum signature
on stdout. Its disconnected handoff is:

```text
Thurin --sign-out
    → typed-data handoff
    → offline / external signer
    → 65-byte Ethereum signature
    → Thurin authorize finish
```

The inspected `authorize finish` path performs signer recovery, checks the
registry nonce, and simulates the matching authorization call before
continuing. Thurin's separate local-GPG preflight behavior is an application
integration concern. Neither it nor registry submission belongs in the
generic trusted signer core.

Mathom remains reference work for the host/offline-signer handoff. Its QR
encoding and asynchronous middleware behavior have not been adopted here.

## Next checkpoints

1. Monitor upstream Keycard Shell releases and test the OpenPGP flow on a
   published firmware build that includes the merged changes.
2. Improve the standalone reference client's usability, QR capture, and
   interoperability testing without weakening the Shell-controlled review
   and signing boundary.
3. Work with Mozilla on Thunderbird WebExtension camera acquisition
   ([bug #2080573](https://bugzilla.mozilla.org/show_bug.cgi?id=2080573));
   maintain external QR capture as an experimental workaround.
4. Investigate trustworthy review and signing for larger messages, with
   explicit display, canonicalization, memory, and multipart QR constraints.
5. Continue independent Ethereum/EVM and Thurin authorization research;
   do not treat working OpenPGP signing as proof of EVM wallet interoperability.
6. Maintain the SAMA5D3 and `KC1` paths as historical research references,
   clearly distinguished from the versioned Shell protocol.

## Reference hardware and verification

The SAMA5D3 setup uses:

- Microchip SAMA5D3 Xplained with Buildroot Linux.
- HID Global OMNIKEY 3x21 USB smart-card reader.
- Programmable NXP JavaCard originally used in the PhononDAO alpha, running
  NeoPGP; this is not a retail Keycard.
- Adafruit 1.3-inch 240×240 ST7789 display, NexiGo N960E USB camera, and a
  3×4 keypad with independent APPROVE and REJECT controls.
- `pcsc-lite`, `libusb`, `ccid`, OpenSC, `libzbar`, and `libqrencode`.

```text
Mac -- Ethernet/SSH for development --> SAMA5D3
                                          |
                                         USB
                                          |
                                       OMNIKEY
                                          |
                                       ISO-7816
                                          |
                                    research NeoPGP card
```

The QR request/response transport is optical; the development connection
shown above is why a final physically air-gapped runtime is still unproven.
The [embedded sources](embedded/sama5d3/) retain the reference implementation;
this repository does not yet provide a turnkey device build and deployment.

To verify the recorded OpenPGP attestation without signing or using a card,
run from the repository root with Node.js/npm installed:

```bash
cd experiments/thurin
npm ci
npx tsx test-hardware-attestation.ts
```

This verifies the saved artifact; it does not reproduce a fresh hardware
operation or prove the unfinished QR/OpenPGP integration. The package's
default `npm test` is a placeholder; run the named script directly.

## Appendix: research identity

The original experiment asked whether one JavaCard-held secp256k1 key could
back an OpenPGP identity while its public point also defines an Ethereum
address. That experiment succeeded.

OpenPGP UID: `terricola-testtt`.

OpenPGP fingerprint:

```text
31CE69D66A5E9DE0F977B59C79BB391497E8E6D4
```

Ethereum address derived from the same public point:

```text
0x9cE2E20FC392304fD1e50541eC67168913B5f3fF
```

```text
one on-card secp256k1 private key
                 |
          public curve point
          /                \
OpenPGP identity      Ethereum address
```

The private key was generated on-card and has not been exported. The Ethereum
address is unfunded and experimental. Sharing a key across protocols was a
research probe, not a deployment recommendation or architectural requirement.
A production design may deliberately separate keys and security domains.

## Appendix: lab setup and observed card results

These are historical research-card setup commands and observations. Applet
installation and key generation change card state and are not required to
verify the saved attestation above.

NeoPGP was installed with secp256k1 enabled using the lab's default
GlobalPlatform management key:

```bash
gp -install NeoPGPApplet.cap \
  -params 02000000 \
  -key 404142434445464748494A4B4C4D4E4F
```

GnuPG reported:

```text
Application type .: OpenPGP
Version ..........: 3.4
Manufacturer .....: NeoPGP
Key attributes ...: secp256k1 secp256k1 secp256k1
```

In the tested setup, `gpg-card` rejected secp256k1 from its key-generation
allowlist, while direct `scdaemon` generation worked:

```bash
gpg-connect-agent "SCD GENKEY OPENPGP.1" /bye
```

The SAMA5D3 detected the reader through PC/SC:

```text
Nr.  Card  Features  Name
0    Yes             HID Global OMNIKEY 3x21 Smart Card Reader
```

Selecting the NeoPGP AID `D2760001240103040010000000000000` returned `90 00`.
Direct card reads returned:

```text
Application identifier:
D2 76 00 01 24 01 03 04 00 10 00 00 00 00 00 00

Signing-key fingerprint:
31 CE 69 D6 6A 5E 9D E0 F9 77 B5 9C 79 BB 39 14
97 E8 E6 D4

Signing algorithm attributes (ECDSA / secp256k1):
13 2B 81 04 00 0A FF
```

The early raw-signature test signed `keycard-openpgp embedded signing test`
through SELECT → VERIFY PIN → PSO: COMPUTE DIGITAL SIGNATURE, returning
64-byte ECDSA `r || s` and status `90 00`. Independent host verification
reported:

```text
VALID: embedded NeoPGP secp256k1 signature verified
```

That establishes the raw hardware primitive. The later OpenPGP milestone
adds the protocol-specific digest and packet construction described above.

## Security and scope

The merged Keycard Shell work does not replace the official Keycard applet
with NeoPGP, turn retail Keycard into a conventional OpenPGP smartcard, or
require sharing one key between OpenPGP and Ethereum. The historical SAMA5D3
experiments are separate from the upstream Keycard Shell implementation.

Hardware testing and upstream merge do not constitute a security audit or
prove that any host integration is production-ready. There is no claim here
of complete wallet interoperability, release availability, production backup
and recovery, or a universally air-gapped runtime. Protocol separation,
trusted display, explicit physical authorization, and a considered recovery
model remain important requirements.

Do not commit private keys, PINs, local GnuPG state, firmware signing keys, or
sensitive development credentials to this repository.

## Documentation and license

- [Upstream Keycard Shell OpenPGP implementation (PR #227)](https://github.com/keycard-tech/keycard-shell/pull/227) — merged firmware changes and hardware validation.
- [Shell OpenPGP protocol v1 (PR #42)](https://github.com/0xterricola/keycard-openpgp/pull/42) — versioned requests and results.
- [Standalone reference client](reference-client/) — experimental QR request/response tooling.
- [GapPGP for Thunderbird](https://github.com/0xterricola/gappgp-thunderbird) — separate signed-email experiment.
- [Roadmap & Project Status](docs/ROADMAP_STATUS.md) — historical milestone tracking; may lag current work.
- [End-to-End QR Signing Prototype](docs/END_TO_END_QR_SIGNING.md) — historical SAMA5D3 demonstration.
- [Keycard Shell EIP-712 Research](docs/research/KEYCARD_SHELL_EIP712.md) — earlier Ethereum source research and open questions.

Licensed under [MIT](LICENSE).
