# keycard-openpgp

**Experimental trusted signing architecture: OpenPGP first, with Keycard Shell as the intended portable interface.**

An application supplies structured intent. A separate trusted device validates
that intent, derives the protocol-specific signing digest itself, presents what
the human is authorizing, and gates a removable secure element's private-key
operation behind physical approval and local PIN entry.

The SAMA5D3 is the current reference device. NeoPGP on a programmable research
JavaCard supplies the hardware key; it is a lab stand-in, not a proposal to
replace the official Keycard applet on retail cards. Keycard Shell is the
portable target, with the official Keycard applet a preferred secure-element
target still under investigation.

The embedded OpenPGP artifact milestone is complete: the recorded hardware
path produced a standard signature accepted by GnuPG and a clear-signed Thurin
attestation verified by `identity-kit`. Integrating that path with the full
trusted QR review loop remains work in progress.

> **Status snapshot — 2026-09-20.** Development moves quickly, and documentation
> can become stale within a day. Status sections describe recorded evidence at
> a checkpoint, not release guarantees. Compare dated prose with current code,
> tests, and tracked artifacts; resolve conflicts using the implementation
> evidence and update the docs at the next checkpoint. The
> [roadmap](docs/ROADMAP_STATUS.md) provides detailed tracking, but can also lag.
> A completed component milestone does not imply a completed end-to-end flow.

**Experimental research only. Do not use this prototype for meaningful funds,
production identities, or sensitive long-lived keys.**

## Architecture

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

These are architectural requirements. The current prototype demonstrates
parts of them; the rendering and integration limitations below remain open.

## Demonstrated status and remaining gaps

| Path | Recorded evidence | Scope |
| --- | --- | --- |
| Connected GnuPG / research card | OpenPGP 3.4 recognition, on-card secp256k1 key, normal GnuPG signatures | Connected smart-card interoperability |
| SAMA5D3 `KC1` QR loop | Camera input, trusted review of demonstrated messages, physical approval/rejection, local keypad PIN, raw signature response | Locally hashes the message with SHA-256; returns raw ECDSA |
| Direct embedded OpenPGP path | Device constructs v4 digest and Signature Packet; NeoPGP signs; GnuPG reports `Good signature` | Fixed-message hardware test, separate from QR review |
| Canonical Thurin OpenPGP artifact | Tracked clear-signed artifact returns `{ verified: true }` with `@thurinlabs/identity-kit` 1.1.1 | OpenPGP attestation verification, not Ethereum authorization |
| Keycard Shell | Source research and a building experimental NeoPGP-detection branch | Custom firmware has not been run on retail Shell |

The interaction tests used the NeoPGP signature counter as hardware evidence:

- Approved request and successful local PIN signing: `10 → 11`.
- PIN-stage cancellation: `11 → 11`.
- Request rejection: counter unchanged.

The current signer uses local keypad PIN entry and masked display feedback;
the terminal PIN dependency has been removed. SSH and Ethernet still serve
development and launch/debugging needs, so the board is not yet demonstrated
as a fully network-disconnected device.

Not yet demonstrated or completed:

- A single structured OpenPGP QR request → trusted review → standard artifact
  response flow, including general text handling and complete rendering.
- A hardware EIP-712 signature through this project's trusted Ethereum
  adapter, or EVM transaction signing through that adapter.
- The full Thurin `--sign-out` → hardware signer → `authorize finish` flow,
  followed by a hardware-authorized Sepolia attestation and registry lookup.
- Generic request correlation, replay protection, persistent pending requests,
  retries, or BC-UR/multipart integration in this prototype.
- Network-free boot/runtime, a working Shell port, or use of the official
  Keycard applet as this project's signing SE.
- Production backup/recovery or a production policy for cross-protocol keys.

## KC1: current QR contract

The current request is:

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

## OpenPGP artifact milestone

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

The next OpenPGP integration is to combine those components with structured
request validation, trusted review, approval, local PIN handling, and a
standard result. Any canonicalization or signature metadata affecting the
digest must be handled inside that trusted protocol adapter.

## Keycard Shell and Ethereum/EVM research

Shell is the intended portable interface because it provides smart-card
transport, a display, physical controls, a camera, and QR support. The
experimental NeoPGP AID-detection branch builds, but it is only a lab step.
Retail Shell requires appropriately signed firmware; this project has not
run its custom firmware on the retail device.

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

1. Close the KC1 parser/renderer mismatch and define complete trusted review
   for the supported request types.
2. Connect the proven OpenPGP digest/packet path to the trusted interaction
   loop through a reusable structured request/result interface.
3. Study/adopt suitable request correlation, replay rules, and QR/UR framing
   without conflating transport with protocol semantics.
4. Implement and test trusted EIP-712 processing, canonical Ethereum
   signatures, and fixed-key identity handling; exercise Thurin's external
   signer flow and then its Sepolia integration.
5. Boot directly into the signer, remove SSH/Ethernet dependencies, and
   demonstrate the network-free runtime.
6. Port the validated interfaces and interaction model to suitable Shell
   development hardware, subject to its firmware-signing constraints.

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

This project does not propose replacing `status-keycard` with NeoPGP, turning
retail Keycard into a conventional OpenPGP smartcard, or requiring the same
key for OpenPGP and Ethereum. It does not claim a retail Shell port, complete
wallet interoperability, or a production security boundary.

Using one private key across OpenPGP and Ethereum collapses security domains.
A production design needs protocol separation, trusted display, explicit
physical authorization, and a considered recovery model.

Do not commit private keys, PINs, local GnuPG state, firmware signing keys, or
sensitive development credentials to this repository.

## Documentation and license

- [Roadmap & Project Status](docs/ROADMAP_STATUS.md) — detailed milestone tracking.
- [End-to-End QR Signing Prototype](docs/END_TO_END_QR_SIGNING.md) — historical
  optical signing demonstration and counter evidence.
- [Keycard Shell EIP-712 Research](docs/research/KEYCARD_SHELL_EIP712.md) —
  inspected source behavior, candidate reuse, and unresolved questions.

Licensed under [MIT](LICENSE).
