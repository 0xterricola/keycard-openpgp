# Keycard Shell EIP-712 Architecture Research

Status: research in progress

This document records observed behavior in the Keycard Shell source before
design decisions are made for keycard-openpgp.

The goal is to distinguish:

- behavior observed directly in Keycard Shell;
- architectural conclusions we can reasonably draw from that behavior;
- components that may be reusable or portable;
- questions that remain unanswered.

Do not treat proposed architecture in this document as implementation
requirements until it is promoted into the project architecture document.

## Reference source

Local repository:

    ~/Developer/keycard-shell

Fork:

    git@github.com:0xterricola/keycard-shell.git

Upstream:

    https://github.com/keycard-tech/keycard-shell.git

Research was performed on:

    branch: feature/openpgp-card
    commit: 1ca841c feat: detect NeoPGP OpenPGP applet

## High-level observed flow

The currently observed Ethereum QR signing path is:

    QR camera
        |
        v
    UR decoder
        |
        v
    ERC-4527 eth-sign-request
        |
        v
    core_eth_eip4527_run()
        |
        +--> Ethereum transaction
        |
        +--> personal/raw message
        |
        `--> EIP-712 typed data
                |
                v
            eip712_hash()
                |
                v
            trusted EIP-712 review
                |
                v
            human approval
                |
                v
            core_eth_sign()
                |
                v
            Keycard secp256k1 signing
                |
                v
            recovery-id resolution
                |
                v
            ERC-4527 eth-signature
                |
                v
            UR QR response

This is important because transport, protocol parsing/hashing, presentation,
card signing, and response encoding are visibly separate concerns.

## 1. QR and UR transport

Relevant files:

    app/qrcode/qrscan.c
    app/ur/ur.c
    app/ur/ur.h
    app/ur/ur_decode.c
    app/ur/ur_decode.h
    app/ur/ur_encode.c
    app/ur/ur_encode.h
    app/ur/ur_types.h
    cddl/ur.cddl

Keycard Shell recognizes the UR type:

    ETH_SIGN_REQUEST

The QR scanner accepts the UR payload and deserializes it using:

    cbor_decode_eth_sign_request()

The resulting request is dispatched by core_qr_run() to:

    core_eth_eip4527_run()

The ERC-4527 structure includes concepts including:

- optional request ID;
- sign data;
- sign-data type;
- optional chain ID;
- derivation path;
- optional address;
- optional request origin.

The response type is:

    ETH_SIGNATURE

The original request ID is copied into the response when it was present in the
request.

### Architectural observation

Request correlation already exists at the Ethereum QR protocol layer. It is not
necessary for the trusted signer to invent an independent Ethereum request-ID
scheme.

This does not by itself solve asynchronous OpenPGP integration, but it provides
a useful existing design pattern.

## 2. ERC-4527 request dispatch

Relevant file:

    app/core/core_eth.c

core_eth_eip4527_run() dispatches according to the request's sign-data type.

Observed paths include:

    Ethereum transaction
    Ethereum typed transaction
    Ethereum raw bytes
    Ethereum typed data

EIP-712 is therefore not its own transport protocol. It is one signing-data
type carried inside the broader Ethereum ERC-4527 request/response protocol.

### Architectural observation

The distinction should be preserved in keycard-openpgp:

    transport/request protocol != EIP-712 protocol engine

A future OpenPGP QR protocol should similarly avoid coupling OpenPGP packet
semantics directly to QR scanning code.

## 3. Card and key selection before signing

Relevant file:

    app/core/core_eth.c

Before processing an ERC-4527 signing request, Shell:

1. applies the requested derivation path;
2. obtains the public key information from the inserted card;
3. derives the Ethereum address locally;
4. obtains the key fingerprint;
5. checks the request's source fingerprint against that locally derived
   fingerprint.

A mismatch aborts the request as a wrong-card condition.

### Architectural observation

The host proposes which key/path it expects.

The trusted signer independently establishes which key is actually present and
rejects requests that target a different identity.

This is stronger than trusting a host-provided address label.

## 4. Generic EIP-712 protocol engine

Relevant files:

    app/ethereum/eip712.c
    app/ethereum/eip712.h

The EIP-712 implementation parses the top-level typed-data components:

    types
    primaryType
    domain
    message

It implements functionality for:

- parsing EIP-712 types;
- finding referenced struct types;
- hashing type definitions;
- encoding individual fields;
- hashing structs;
- hashing the EIP712Domain;
- hashing the selected primary type and message;
- reconstructing the parsed typed data for presentation.

The public hashing entry point is:

    eip712_hash()

core_eth_process_eip712() initializes the Ethereum EIP-712 prefix:

    0x19 0x01

It then calls eip712_hash() and finalizes the resulting Keccak state locally.

### Architectural observation

The computer does not provide an opaque final EIP-712 digest for Shell to
blindly sign.

The trusted device receives the structured EIP-712 data and independently
derives the digest that will be signed.

This should be treated as a major trust-boundary requirement for
keycard-openpgp's Ethereum support.

## 5. Shared parsed state for hashing and review

Relevant files:

    app/core/core_eth.c
    app/ethereum/eip712.c
    app/ui/dialog.c

core_eth_process_eip712() stores parsed EIP-712 state in:

    g_core.data.eip712

The same context contains the locally calculated hash and is subsequently
passed into the trusted EIP-712 display path.

### Architectural observation

Cryptographic interpretation and human review are tied to the same parsed
request representation.

This is preferable to:

    host display description
        +
    unrelated host-provided digest

because those two host-controlled values could disagree.

A similar property should be sought for OpenPGP.

## 6. Semantic recognition and generic EIP-712

Relevant files:

    app/ethereum/eth_data.c
    app/ethereum/eth_data.h
    app/ui/dialog.c

Current recognized EIP-712 semantic types include:

    Permit
    PermitSingle
    SafeTx

Recognition is performed by:

    eip712_recognize()

Recognized structures receive specialized extraction and confirmation UI.

If the EIP-712 structure is not recognized, Shell does not automatically fall
back to signing only an opaque hash.

Instead it:

1. extracts the EIP-712 domain;
2. serializes the parsed EIP-712 object for display;
3. presents a generic raw EIP-712 confirmation UI;
4. includes the locally calculated digest in the review context.

### Architectural observation

Shell has two presentation levels:

    generic EIP-712
        +
    optional richer semantic recognition

An application such as Thurin therefore does not need to become a new signing
protocol.

Its typed data can first work through generic EIP-712.

A future Thurin-specific recognizer may provide a better trusted presentation,
analogous to Permit or SafeTx, without changing the underlying EIP-712 engine.

## 7. Trusted approval occurs before the card operation

Relevant files:

    app/core/core_eth.c
    app/ui/dialog.c

The EIP-712 data is parsed and hashed and the confirmation UI runs before
core_eth_sign() is called.

Cancellation returns without performing the card signature operation.

### Architectural observation

The intended ordering is:

    parse
        ->
    independently derive signing input
        ->
    trusted review
        ->
    human approval
        ->
    private-key operation

This ordering should be preserved across protocol engines.

## 8. The card signing boundary

Relevant files:

    app/core/core_eth.c
    app/keycard/keycard_cmdset.c
    app/keycard/keycard.c

core_eth_sign() finalizes the local Keccak state into a 32-byte digest.

That digest is passed to:

    keycard_cmd_sign(
        ...,
        KEYCARD_SIGN_ECDSA_SECP256K1,
        ...,
        digest
    )

The private-key operation therefore occurs below the Ethereum parsing,
presentation, and approval layers.

### Architectural observation

The card may remain comparatively protocol-agnostic.

The trusted host device is responsible for establishing the legitimacy and
meaning of the digest before invoking the private-key primitive.

For keycard-openpgp, a generic internal digest-signing primitive can therefore
be appropriate provided it is not exposed directly as an unrestricted
host-facing signing command.

## 9. ECDSA signature recovery

Relevant file:

    app/keycard/keycard.c

keycard_read_signature() supports more than one card-response representation.

When it receives the form containing a public key and DER ECDSA signature, it:

1. converts the DER signature into raw r and s;
2. attempts secp256k1 public-key recovery for recovery IDs 0 through 3;
3. compares each recovered public key against the public key supplied by the
   card;
4. selects the matching recovery ID;
5. stores that ID in byte 64 of the output signature.

This means the card itself does not necessarily have to provide Ethereum's
recovery identifier directly.

### Architectural observation

This is highly relevant to the current NeoPGP experiment.

If NeoPGP returns only r || s, the trusted device may be able to derive the
Ethereum recovery ID independently using the known public key and signed
digest.

The exact compatibility of this Shell implementation with the NeoPGP response
still needs to be tested rather than assumed.

## 10. Ethereum response construction

Relevant file:

    app/core/core_eth.c

After signing, Shell builds an eth-signature response.

The response:

- carries the incoming request ID when present;
- contains the signature;
- applies Ethereum v handling appropriate to the operation;
- is CBOR encoded;
- is presented as an ETH_SIGNATURE UR QR.

### Architectural observation

The online wallet remains responsible for receiving the standard Ethereum
signature response and continuing the network-facing workflow.

The offline device does not need to understand the wallet's broadcasting or
dApp lifecycle.

## 11. Chain ID observations

The ERC-4527 envelope contains an optional chain ID.

In the source inspected so far, the outer request chain ID is explicitly
applied to Ethereum transaction processing.

The EIP-712 path instead processes the typed-data JSON, whose domain may itself
contain chainId and which participates in the EIP-712 hash.

No additional comparison between the outer ERC-4527 chain ID and the EIP-712
domain chainId has yet been observed in this research.

Do not claim that Shell performs such a cross-check unless source evidence is
found later.

## 12. Current architectural model suggested by the research

The current evidence supports the following conceptual layering:

    Application / online wallet
                |
                v
    Protocol request/response framing
          e.g. ERC-4527
                |
                v
    Transport encoding
          UR / CBOR / QR
                |
                v
    Protocol engine
          e.g. EIP-712
                |
                v
    Trusted presentation / semantic policy
          generic or recognized
                |
                v
    Human approval
                |
                v
    Card/private-key primitive
                |
                v
    Protocol response

These are conceptual boundaries, not yet a finalized keycard-openpgp API.

## 13. Comparison target for OpenPGP

A possible analogous OpenPGP shape is:

    host application / connector
                |
                v
    OpenPGP signing request
                |
                v
    transport
                |
                v
    OpenPGP protocol engine
                |
                v
    trusted OpenPGP presentation
                |
                v
    human approval
                |
                v
    card/private-key primitive
                |
                v
    standards-compliant OpenPGP artifact

The exact OpenPGP request/response protocol and asynchronous host integration
remain open design questions.

They should not be invented merely to resemble Ethereum unless doing so also
fits OpenPGP's real requirements.

## 14. Thurin implications

Thurin should currently be considered:

    an application using OpenPGP and EIP-712

rather than:

    a new signing protocol

Its EIP-712 authorization should be able to pass through the generic EIP-712
engine.

A future semantic recognizer for a Thurin Attest structure could provide
specialized trusted wording without changing EIP-712 hashing semantics.

The current Thurin CLI's external Ethereum signer boundary and its separate
local-GPG preflight behavior remain an application-integration concern outside
the core EIP-712 engine.

## 15. Candidate reuse classification

### Strong candidates to study for port/reuse

- EIP-712 parsing and hashing code;
- ERC-4527 CDDL definitions;
- UR request/response framing;
- recovery-ID derivation strategy;
- generic-versus-recognized EIP-712 presentation model.

### Likely Shell-specific adaptation

- display implementation;
- STM32/FreeRTOS integration;
- Keycard HD derivation behavior;
- card APDU command implementation;
- camera/display plumbing.

### Reference only until tested

- assumptions about NeoPGP signature response format;
- applicability of Shell derivation-path logic to a single NeoPGP key;
- exact low-s behavior;
- application-specific semantic recognizers.

## 16. Open research questions

- Does the EIP-712 implementation exactly match current Ethereum reference
  vectors for all supported primitive and nested types?
- Where and how is low-s normalization guaranteed?
- Does the Keycard applet itself guarantee canonical low-s ECDSA signatures?
- How should a NeoPGP key with no HD derivation semantics fit into an
  ERC-4527 request that includes a derivation path?
- Should a NeoPGP-backed Ethereum mode accept a fixed/synthetic path or require
  a protocol extension?
- Is the optional ERC-4527 address checked anywhere beyond the source
  fingerprint check?
- Should the outer ERC-4527 chain ID be cross-checked against an EIP-712
  domain chainId?
- Which Shell EIP-712 source files can be reused directly under their existing
  license versus ported with adaptation?
- What test vectors does Keycard Shell use for EIP-712?
- Can the SAMA prototype interoperate directly with MetaMask/Rabby QR flows
  once ERC-4527 and UR support are present?
- What is the clean OpenPGP analogue of ERC-4527 request/response framing?
- How should asynchronous OpenPGP host applications correlate requests and
  responses?
- Which boundaries remain common when a third protocol such as SSH or Nostr is
  added?

## Research principle

The immediate goal is not to generalize prematurely.

Use Ethereum/EIP-712 as protocol #2 to test whether the abstractions learned
from OpenPGP survive a genuinely different signing protocol.

Only boundaries supported by multiple real protocol flows should be promoted
into the durable signing architecture.
## 17. Fixed-key identity versus HD derivation

Observed in Keycard Shell source:

- `core_set_derivation_path()` accepts a crypto-keypath with zero path components. In that case the internal path length becomes zero, the component loop performs no iterations, and the function returns success.
- `core_export_public()` requests the fingerprint with `core_get_fingerprint(path, 0, ...)`, so the fingerprint checked by the Ethereum ERC-4527 path is the root/master fingerprint rather than the fingerprint of the requested child path.
- The fingerprint is formed from the first four bytes of HASH160 of the compressed secp256k1 public key.
- The ERC-4527 Ethereum path separately requires the request's source fingerprint to be present and equal to the locally derived fingerprint.

Architectural consequence:

HD derivation is not itself the general security boundary. The general boundary is that the request identifies an intended signing identity and the trusted device independently proves that the inserted key matches that identity.

A fixed NeoPGP secp256k1 key may therefore fit this model without inventing a fake BIP-44 hierarchy. A possible compatibility mapping is:

    empty crypto-keypath
        +
    stable four-byte source fingerprint derived from the fixed public key

This is only a candidate mapping. Compatibility with wallet account-import flows such as MetaMask still needs to be verified because those flows may impose additional HD-key or derivation-path expectations.

The four-byte ERC-4527/BIP-32-style source fingerprint must not be confused with the OpenPGP fingerprint. They are different identifiers derived for different protocol purposes.

## 18. Low-s ECDSA behavior

Observed across Keycard Shell and the Keycard JavaCard applet:

- `core_eth_sign()` finalizes a 32-byte digest and sends it to the card with `keycard_cmd_sign()`.
- `keycard_cmd_sign()` forwards the digest and derivation path without normalizing the returned signature.
- `ecdsa_sig_from_der()` parses DER r and s values without low-s normalization.
- `keycard_read_signature()` derives a recovery ID when necessary but does not normalize s.
- The Keycard applet's `SECP256k1.ecdsaSign()` directly invokes JavaCard `Signature.signPreComputedHash()`.
- `SECP256k1.signHash()` returns that ECDSA result unchanged.
- No applet-level comparison of s against half the secp256k1 group order was found.

Conclusion:

Keycard does not enforce Ethereum low-s canonicalization in the Shell or applet code inspected here.

A particular JavaCard cryptographic provider may emit low-s signatures, but that behavior is not an architectural guarantee supplied by the Keycard implementation.

For a portable Ethereum signing path, low-s handling therefore belongs in the trusted Ethereum protocol layer above the raw card operation.

The intended post-signing flow is:

    raw r || s from card
            |
            v
    if s > n / 2:
        s = n - s
            |
            v
    derive or adjust recovery ID
            |
            v
    recover expected public key
            |
            v
    verify expected Ethereum address
            |
            v
    return canonical 65-byte Ethereum signature

This keeps Ethereum-specific canonicalization outside the generic card signing primitive.

For the NeoPGP experiment, the existing raw `r || s` result is therefore sufficient as a card primitive. The trusted device can perform Ethereum canonicalization and recovery before returning an Ethereum signature.

## 19. Ethereum account export is HD-specific

Observed in Keycard Shell source:

- `get_hd_key()` constructs a Blockchain Commons `crypto-hdkey`.
- The exported object contains a 33-byte compressed public key.
- It also contains a 32-byte chain code.
- It carries an origin crypto-keypath and source fingerprint.
- `core_display_public_eip4527()` exports Ethereum using `BIP44_ETH_PATH`.
- That Ethereum account is encoded as `CRYPTO_HDKEY`.
- The multicoin account export similarly includes Ethereum through `BIP44_ETH_PATH` inside `CRYPTO_MULTI_ACCOUNTS`.

Architectural consequence:

The Shell signing path and the Shell account-discovery path have different requirements.

A fixed secp256k1 key can potentially satisfy a signing request without having HD derivation semantics, but the existing Shell Ethereum account-export protocol expects HD material including a chain code.

Therefore:

    signing compatibility
        !=
    wallet account-export compatibility

The NeoPGP research key has no native BIP-32 chain code.

A synthetic chain code should not be invented merely to satisfy the `crypto-hdkey` schema because that would misrepresent the capabilities and derivation semantics of the underlying key.

The current fixed-key experiment should instead treat account discovery as a separate compatibility problem.

Possible future approaches include:

- allowing the online wallet to already know the fixed Ethereum address and public key;
- defining or using a non-HD public-key account exchange if an existing compatible standard exists;
- using an HD-capable secure element for standard Shell wallet pairing while keeping the trusted protocol engine independent of HD semantics.

No choice is made here yet.

## 20. Boundary clarified by account export

The research now suggests that HD derivation belongs to a wallet/account-management layer rather than the generic trusted-signing architecture.

The durable signing boundary is:

    structured protocol request
        ->
    trusted parse and digest derivation
        ->
    trusted review
        ->
    approval
        ->
    selected signing identity
        ->
    secure-element signing primitive

How a host initially discovers that identity is a separate concern.

For the official Keycard applet, account discovery naturally uses BIP-32 and `crypto-hdkey`.

For the fixed NeoPGP research key, another discovery mechanism may be needed.

This distinction should be preserved when the durable signing architecture document is written.
