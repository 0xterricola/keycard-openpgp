# Keycard OpenPGP — Roadmap & Project Status

_Status snapshot: 2026-09-20_

This document tracks the experimental trusted signing architecture, its
recorded milestones, current limitations, and next checkpoints. OpenPGP is
the first protocol adapter; SAMA5D3 is the reference device; Keycard Shell is
the intended portable interface.

> Development moves quickly, and documentation can become stale within a day.
> This roadmap and the [README](../README.md) are point-in-time summaries,
> not release guarantees. Compare dated prose with current code, tests, and
> tracked artifacts, and reconcile conflicting claims at each checkpoint.
> A completed component milestone does not imply a completed end-to-end flow.
> Results below are recorded evidence, not a claim that every test was rerun
> when this document was updated.

**Experimental research only. Do not use this prototype for meaningful funds,
production identities, or sensitive long-lived keys.**

## Status Legend

- ✅ Demonstrated or completed at the stated scope
- 🟡 Components work; integration is incomplete
- ⏳ Pending implementation or demonstration
- 🔬 Research / architecture investigation
- ⚠️ Known issue or limitation

Source inspection of Shell or Thurin establishes observed behavior there; it
does not mean the corresponding feature is implemented in this project.

## 1. Application / Use Case — Thurin

Thurin is the first external application integration. It uses two distinct
protocol operations: an OpenPGP identity proof and Ethereum EIP-712
authorization. Thurin is an application, not a signing protocol.

### Completed and recorded

- ✅ `@thurinlabs/identity-kit` supports secp256k1 OpenPGP verification
- ✅ Thurin experiment updated and tested with `identity-kit` 1.1.1
- ✅ All five existing Thurin/OpenPGP regression scripts recorded as passing
  on 1.1.1
- ✅ Existing hardware-signed fixture returns `{ verified: true }`
- ✅ Direct embedded hardware attestation also verifies with `identity-kit`;
  see the [tracked artifact](../artifacts/hardware-thurin-attestation.asc)
  and [verification script](../experiments/thurin/test-hardware-attestation.ts)
- ✅ React-free `/core` interface identified
- ✅ Canonical statement identified:

  ```text
  I control the Ethereum address: 0x9ce2e20fc392304fd1e50541ec67168913b5f3ff
  ```

- ✅ Thurin PGP verification path and CLI signing boundaries inspected;
  PGP secret material remains outside Thurin, whose CLI invokes `gpg`
- ✅ `--no-key` external-wallet handoff and `--authorize` EIP-712 path inspected
- ✅ Registry contract recorded in the integration research:
  `0x9302E02e2869e129aC8516fE5eFFd51EA3082c09`
- ✅ Thurin 0.7.0 external signer interface inspected: typed EIP-712 JSON on
  stdin; a 65-byte recoverable Ethereum signature on stdout
- ✅ `--sign-out` handoff and `authorize finish` recovery, registry-nonce check,
  and simulation path inspected

### Remaining integration

- ⏳ Produce a hardware-backed EIP-712 signature through this project's trusted
  Ethereum adapter, initially using the research SE
- ⏳ Complete `--sign-out` → offline signer → `authorize finish`
- ⏳ Resolve the separate local-GPG preflight behavior in the application
  integration without moving application orchestration into the signer
- ⏳ Publish a hardware-authorized attestation on Sepolia and verify the final
  registry/browser result

### Intended Ethereum authorization flow

```text
Thurin prepares structured EIP-712 authorization
    → external signer / sign-out handoff
    → request framing and transport
    → trusted Ethereum adapter parses, validates, and derives the digest
    → trusted review, physical approval, and local PIN
    → SE private-key operation
    → trusted Ethereum canonicalization and recovery checks
    → standard 65-byte signature returned
    → Thurin recovery / nonce check / simulation
    → relayer or funded wallet submits the authorization
    → registry / browser lookup
```

The verified OpenPGP attestation is a separate input to the application flow.
Its completion does not establish that Ethereum authorization has succeeded.

## 2. Protocol Adapters — OpenPGP First

### Completed OpenPGP milestone

- ✅ Research NeoPGP secp256k1 key generated on-card; public key available and
  private key not exported
- ✅ OpenPGP fingerprint:
  `31CE69D66A5E9DE0F977B59C79BB391497E8E6D4`
- ✅ Same public point maps to Ethereum address:
  `0x9ce2e20fc392304fd1e50541ec67168913b5f3ff`
- ✅ Connected GnuPG hardware OpenPGP signing works through the smart-card stack
- ✅ Direct embedded raw ECDSA signing returns 64-byte `r || s`
- ✅ SAMA5D3 constructs the proper OpenPGP v4 digest, including signed data,
  signature fields, and trailer
- ✅ NeoPGP signs that device-derived digest; the embedded code converts the
  raw ECDSA result to MPIs and builds an OpenPGP Signature Packet
- ✅ Hardware-produced detached signature recorded as accepted by GnuPG with
  `Good signature`
- ✅ Canonical Thurin statement signed through the direct embedded path and
  assembled into a tracked clear-signed artifact
- ✅ `@thurinlabs/identity-kit` 1.1.1 returns `{ verified: true }` for that artifact

Evidence lives in the [OpenPGP helper](../embedded/sama5d3/openpgp_v4.c),
[hardware test](../embedded/sama5d3/openpgp_v4_sign_test.c),
[public certificate](../artifacts/terricola-testtt.asc), and artifact/test
linked above. The standards-compliant embedded OpenPGP artifact milestone is
complete.

### Scope and unfinished integration

- 🟡 OpenPGP digest/packet components work, but are not yet integrated with the
  QR request-review-approval flow
- ⚠️ The hardware test uses a fixed message, identity, and creation time; it
  does not run the QR message-review workflow
- ⚠️ The v4 helper consumes supplied signed bytes; it is not a general
  cleartext canonicalizer or clear-signing formatter
- ⚠️ The current `KC1` application still calls `neopgp_sign_message()`, which
  derives `SHA-256(message)` locally and returns raw ECDSA. Its `PGP_SIGN`
  label does not mean it returns a standard OpenPGP artifact
- ⏳ Define a reusable structured OpenPGP signing request/result interface
- ⏳ Handle canonicalization, supported signature metadata, identity selection,
  and complete trusted review inside the OpenPGP adapter
- ⏳ Demonstrate QR request → trusted OpenPGP processing → approval/PIN →
  standard artifact response and independent verification

A signature over plain `SHA-256(message)` cannot simply be wrapped into an
OpenPGP signature: the protocol's signed fields and trailer must already have
participated in the digest before the card signs it.

### Ethereum/EVM and later adapters

- 🔬 EIP-712 is the next protocol operation under study; Shell source research
  is recorded, but this project's trusted adapter is not yet implemented
- ⏳ Implement EIP-712 parsing, local digest derivation, trusted review, low-s
  handling, recovery-ID derivation, and expected-key/address verification
- ⏳ Add Ethereum transactions and messages as sibling operations
- 🔬 Consider SSH, Nostr, Git/release signing, and other structured formats
  after the first two protocol families establish reusable boundaries

### Core design rule

**The trusted device must independently derive the protocol-specific bytes or
digest from the validated structured request.** It must use the same
interpretation for trusted review and signing, and reject requests whose
displayed meaning cannot be bound to the signing input.

Protocol adapters live outside the **generic signer core**, but their
security-critical parsing, encoding, hashing, and presentation logic live
**inside the trusted-device boundary**. An internal digest-signing primitive
does not justify an unrestricted host-facing opaque-digest service.

Application orchestration, network access, and registry submission stay
outside the trusted signer. Optional application-aware semantic recognizers
may improve trusted presentation without becoming new signing protocols.

## 3. Request/Response Middleware and QR Transport

`KC1` is the current prototype envelope. QR is the optical transport. OpenPGP
and EIP-712 define signing semantics. Keep these layers separate.

### Current prototype

- ✅ Camera reads QR requests; device renders QR signature responses
- ✅ Sequential optical request → physical approval/local PIN → hardware
  signing → optical response demonstrated
- ✅ Request form: `KC1|OP=PGP_SIGN|MSG=<message>`
- ✅ Response form: `KC1|FP=<fingerprint>|SIG=<raw-signature-hex>`
- ✅ Device hashes the same parsed message buffer supplied to the renderer
- ✅ Display application rejects messages longer than 20 characters before
  signing; the parser's separate input limit is 160 printable ASCII bytes
- ⚠️ Accepted printable ASCII exceeds the font's supported glyphs. The font
  renders uppercase letters, digits, spaces, `.`, `-`, and `:`; other accepted
  characters render blank. The length check does not prove faithful display
  for every accepted request
- ⚠️ No request ID, nonce, timestamp, or intended-key selector in the request;
  no explicit request ID or digest in the response
- ⚠️ No implemented KC1 replay protection; returned fingerprint metadata is
  not request correlation or proof of an expected-key check
- ⚠️ Optical transport is demonstrated while SSH/Ethernet remain available for
  development. A final network-free runtime is still pending

See the [current parser](../embedded/sama5d3/qr_request.c) and
[interaction application](../embedded/sama5d3/sign_prompt_v4.c).
The [end-to-end report](END_TO_END_QR_SIGNING.md) preserves the original
demonstration; its terminal-PIN limitation predates the local keypad work.

### Research and remaining work

- ✅ Shell's ERC-4527 Ethereum request/response framing and UR/CBOR QR path
  inspected and documented
- 🔬 Study reuse/adaptation of Shell transport and multipart support
- 🔬 Study Mathom's host/offline-signer boundary; its encoding and asynchronous
  middleware have not been adopted
- ⏳ Resolve the KC1 parser/renderer mismatch and define complete review for
  each supported request type
- ⏳ Define/adopt protocol and version identifiers, canonical request encoding,
  intended-key selection, request correlation, and response validation
- ⏳ Define protocol-appropriate replay/nonce rules; request IDs alone are not
  replay protection
- ⏳ Define persistent pending requests, multiple outstanding operations,
  retries, and application-facing middleware interfaces where needed
- ⏳ Replace prototype-specific KC1 framing and adopt BC-UR/multipart transport
  if supported by interoperability requirements

Shell's optional request-ID echo is evidence about its Ethereum framing,
not evidence that request correlation or replay handling is complete here.
The trusted protocol adapter receives structured requests after transport
decoding; the boundary is shown in section 6.

## 4. Trusted Signer / Hardware Prototype

The current reference hardware is the Microchip SAMA5D3 Xplained. NeoPGP on
the programmable research JavaCard supplies a real non-exportable hardware
key while the trusted-device architecture is developed. It is not a retail
Keycard or a proposed replacement for the official Keycard applet.

### Working now

- ✅ SAMA5D3 prototype boots
- ✅ ST7789 display, camera, QR decoding, and USB smart-card reader work
- ✅ NeoPGP communication and hardware private-key operations work
- ✅ Demonstrated messages can be reviewed on the device, subject to the
  character-rendering limitation in section 3
- ✅ Physical APPROVE / REJECT controls gate the QR signing operation
- ✅ Signature-counter evidence:
  - Request REJECT: unchanged
  - PIN-stage cancellation: `11 → 11`
  - Approved request and successful local PIN signing: `10 → 11`

### Keypad / local PIN

- ✅ Full 12-button matrix electrically and logically working
- ✅ Reliable `0–9` input and independent APPROVE / REJECT buttons
- ✅ R1 moved from the non-working PD30 path to PA16
- ✅ Local NeoPGP PIN entry through the physical keypad
- ✅ Masked PIN progress on the trusted display
- ✅ PIN digits not echoed or logged
- ✅ Local PIN input buffer cleared on cancellation/failure and after VERIFY;
  the VERIFY command's PIN copy is cleared after its transport call succeeds
- ⚠️ A VERIFY transport failure skips clearing the command buffer's PIN copy;
  do not claim all PIN copies are cleared on every exit path
- ✅ PIN-stage REJECT fails closed without signing
- ✅ One VERIFY attempt per signing attempt; failure exits closed
- ✅ Terminal PIN dependency removed

These interaction results concern the QR prototype. The separate fixed-message
OpenPGP hardware test does not yet integrate the complete review/approval UI.

### Remaining hardware hardening

- ⏳ Clear every PIN copy on all exits, including VERIFY transport failure
- ⏳ Boot directly into the signer application
- ⏳ Remove SSH launch/control dependency
- ⏳ Physically disconnect Ethernet
- ⏳ Demonstrate the complete network-free runtime
- ⏳ Define production recovery and backup requirements

Local PIN entry is complete; SSH still serves development and debugging.
Removing the terminal PIN dependency did not remove the network connection.

## 5. Target Hardware — Keycard Shell

Shell is the intended portable UI and transport device. The official Keycard
applet is a preferred SE target under investigation, while NeoPGP remains the
lab stand-in. Shared-key use across OpenPGP and Ethereum is not required.

### Source research completed

The [Shell/EIP-712 research](research/KEYCARD_SHELL_EIP712.md) records inspection
of the `feature/openpgp-card` branch at `1ca841c`:

- ✅ ERC-4527 `eth-sign-request` / `eth-signature` over UR/CBOR QR inspected
- ✅ EIP-712 parsing and device-local digest construction inspected
- ✅ Shared parsed state for hashing and review identified
- ✅ Generic typed-data review and recognized presentations such as Permit
  and SafeTx identified
- ✅ Approval-before-card-signing ordering inspected
- ✅ Requested source-fingerprint check and optional request-ID echo inspected
- ✅ Signature recovery-ID derivation strategy inspected
- ✅ Low-s limitation recorded: the inspected Shell/applet implementation does
  not provide a portable canonicalization guarantee
- ✅ HD account export identified as a separate compatibility concern;
  `crypto-hdkey` includes public key, chain code, and origin metadata
- ✅ Experimental NeoPGP AID-detection firmware recorded as building

These checkmarks describe inspected source and the build milestone, not a
working OpenPGP or NeoPGP signing port on Shell. Custom firmware has not run
on retail Shell; its signed-firmware constraints remain relevant.

### Open questions and implementation

- 🔬 Assess code reuse/licensing and EIP-712 reference-vector coverage
- 🔬 Evaluate fixed-key identity mapping: an empty keypath plus a source
  fingerprint is only a candidate, not demonstrated wallet interoperability
- 🔬 Resolve account discovery separately from signing. The fixed NeoPGP key
  has no HD chain code; do not invent one to imitate `crypto-hdkey`
- 🔬 Keep the four-byte source fingerprint distinct from an OpenPGP fingerprint
- 🔬 Establish chain-ID consistency rules. No outer ERC-4527 chain-ID versus
  EIP-712 domain `chainId` cross-check was established by the source review
- ⏳ Test low-s normalization, recovery, and expected identity verification for
  raw NeoPGP signatures in the trusted Ethereum adapter
- ⏳ Test fixed-key wallet pairing and signing interoperability
- ⏳ Map the generic core, protocol adapters, rendering, approval/PIN UI, and
  transport interfaces onto suitable Shell development hardware
- ⏳ Integrate and test the chosen SE backend; AID detection alone is not a port
- ⏳ Demonstrate the complete hardware-independent signing architecture

## 6. General Architecture

```text
APPLICATION / HOST CONNECTOR
Thurin / GnuPG / wallets / future applications
    builds structured requests; consumes and verifies standard results
                         |
             REQUEST/RESPONSE FRAMING
             KC1 prototype; future protocol-appropriate formats
                         |
                   QR / TRANSPORT
                         |
    +--------- TRUSTED DEVICE BOUNDARY ---------+
    |                                          |
    |  TRUSTED PROTOCOL ADAPTER                 |
    |  OpenPGP / Ethereum-EIP-712 / future      |
    |    parse and validate                    |
    |    derive protocol-specific bytes/digest |
    |    provide the same meaning for review   |
    |                    |                     |
    |  TRUSTED PRESENTATION / POLICY            |
    |                    |                     |
    |  GENERIC SIGNER CORE                      |
    |  lifecycle / physical approval / PIN      |
    |  card interface ---------------------------> REMOVABLE SE
    |                 <--------------------------- private-key operation
    |  TRUSTED ADAPTER constructs standard result|
    +--------------------|---------------------+
                         |
                  RESPONSE TRANSPORT
                         |
                    APPLICATION
```

The generic signer core and trusted protocol adapters are distinct software
layers, both inside the trusted-device boundary. Protocol-specific digest
derivation is never delegated to an untrusted host as an opaque signing input.

The host may assemble wrappers around the returned result. It cannot choose
signing bytes or metadata affecting the signed meaning unless the trusted
adapter independently validates them and includes them in the protocol-defined
digest. Hardware identity/account discovery is separate from protocol hashing;
the trusted signer still needs to establish the intended signing identity.

SAMA5D3 is the current reference implementation platform; Shell is the intended
portable target. The diagram is the architectural requirement, not a claim
that every path is integrated today.

## 7. Immediate Priorities

### Recently completed

- ✅ Keypad matrix debugging, including the R1 / PA16 correction
- ✅ Local PIN entry, masked feedback, and fail-closed cancellation
- ✅ Working SAMA sources retained under `embedded/sama5d3/`
- ✅ Thurin experiment regression-tested on `identity-kit` 1.1.1
- ✅ Thurin 0.7.0 external signer and sign-out boundaries inspected
- ✅ Direct embedded OpenPGP digest/packet path implemented
- ✅ Hardware-produced OpenPGP signature accepted by GnuPG; canonical
  clear-signed attestation verified by `identity-kit`
- ✅ Shell/EIP-712 source research recorded, including fixed-key/HD discovery
  distinctions and Ethereum signature-processing requirements

### Next checkpoints and completion evidence

1. ⏳ **Faithful trusted rendering:** reconcile the KC1 parser/font mismatch;
   demonstrate that every accepted character is rendered correctly and
   unsupported or overlength requests fail before signing. Also close the
   known VERIFY failure-path PIN cleanup gap and verify all sensitive copies
   are cleared on every exit.
2. ⏳ **Integrated OpenPGP interface:** define structured requests and standard
   results; demonstrate trusted canonicalization/digest derivation, complete
   review, approval/local PIN, hardware signing, and independent verification
   through the same QR flow.
3. ⏳ **Request/response middleware:** adopt framing and transport suited to the
   protocols, then demonstrate intended-key checks, correlation, response
   validation, and the chosen replay rules.
4. ⏳ **Trusted Ethereum adapter:** verify EIP-712 vectors, low-s handling,
   recovery, and expected-key/address checks; produce a hardware signature
   through Thurin's external/offline signer interface.
5. ⏳ **Thurin publication:** complete `authorize finish`, publish on Sepolia,
   and verify the registry/browser result.
6. ⏳ **Network-free reference device:** boot into the signer and demonstrate
   the complete interaction with SSH/Ethernet removed.
7. ⏳ **Shell port:** exercise the validated interfaces on suitable development
   hardware, including the selected SE backend and firmware constraints.

## 8. Recorded Milestones

- ✅ On-card secp256k1 key generation; private key not exported
- ✅ One public point mapped to OpenPGP identity and Ethereum address as a
  research experiment, not a recommended production key policy
- ✅ Connected GnuPG hardware signing
- ✅ Camera-driven QR input, physical approval/rejection, and raw signature QR
  output in the reference prototype
- ✅ Full keypad and local masked PIN interaction; no terminal PIN dependency
- ✅ Signature-counter evidence of no signing on request rejection or PIN
  cancellation, and signing on the approved successful path
- ✅ Device-derived OpenPGP v4 digest, hardware ECDSA, and Signature Packet
- ✅ GnuPG verification of a hardware-produced signature and `identity-kit`
  verification of the tracked canonical Thurin clear-signed artifact
- ✅ secp256k1 verification support landed in Thurin identity tooling; Ben's
  compatibility PR merged
- ✅ Thurin experiment updated and regression-tested on `identity-kit` 1.1.1
- ✅ Shell source research and experimental NeoPGP-detection build recorded
- ✅ Repository licensed under [MIT](../LICENSE)

The QR interaction loop and the direct OpenPGP artifact path are separately
demonstrated components. Their integration, faithful rendering for all accepted
input, Ethereum authorization, and final network isolation remain open.

## 9. Guiding Security Properties

- The private key stays on the hardware card.
- The trusted device independently parses the structured request and derives
  the protocol-specific bytes or digest to be signed.
- Trusted review and signing use the same validated interpretation; the host
  cannot supply unrelated display text and an opaque digest.
- Unsupported or unrenderable requests are rejected rather than truncated,
  guessed, or approved with only an opaque hash shown.
- Physical rejection, PIN cancellation, and failed PIN verification prevent
  the signing operation.
- PIN entry stays on the trusted device; only masked progress is shown. PIN
  digits are never logged or returned over the transport.
- The trusted device must establish the intended signing identity. Returned
  fingerprint metadata alone is not an expected-key check.
- Protocol-specific parsing, encoding, hashing, and signature processing
  belong outside the generic signer core but inside the trusted-device boundary.
- Applications should receive standard cryptographic results without needing
  to know which hardware produced them.
- Correlation, replay handling, and identity/account discovery are separate
  concerns with explicit protocol-specific requirements.
- Shared OpenPGP/Ethereum keys are a research probe. Production key separation
  and recovery policy remain design work.

These requirements guide the work; the known limitations above identify where
the prototype does not yet satisfy them in full.
