# Keycard Shell OpenPGP → Ethereum → Thurin End-to-End Demo

## Status

This demo exercises a complete hardware-backed identity authorization flow using
one physical Keycard with two distinct cryptographic identities:

- an OpenPGP identity controlled by a non-exportable Keycard key; and
- an Ethereum account controlled by the normal Ethereum key on the same Keycard.

The flow proves that the OpenPGP identity can make a signed claim about an
Ethereum address and that the Ethereum account can independently authorize the
same Thurin identity operation.

Neither private key is exported to the host computer.

The current demo uses Ethereum mainnet.

---

## Recorded demo profile

The recorded demonstration intentionally uses two independent Keycard-backed
identities:

    OpenPGP identity: stable index 0
    Ethereum address: index 1
    Ethereum path:    m/44'/60'/0'/0/1

The OpenPGP identity is the existing stable Shell-controlled identity. Its
public certificate and fingerprint remain unchanged.

The Ethereum side intentionally uses address index 1 so the recorded flow does
not reuse the already-published Ethereum index-0 Thurin claim.

The live demonstration performs the complete hardware-backed cryptographic
flow:

    runtime Ethereum address capture
    ERC-4527 account metadata capture
    OpenPGP SIGN_MESSAGE
    local OpenPGP-to-Ethereum verification
    Thurin EIP-712 authorization generation
    Ethereum hardware signature
    local Ethereum signer recovery
    Thurin no-relay handoff generation

The live index-1 authorization is not published to Mainnet.

The generated Thurin handoff URL is a signed authorization artifact and is
hidden from the recording.

After the live flow completes, the demonstration switches to a previously
completed Mainnet reference using Ethereum address index 0:

    Ethereum address:
    0xB8A0f79E6d64c948E9F5ea32aD93647777915EBa

    Etherscan transaction:
    https://etherscan.io/tx/0xddd7085ccc4f33865477eabaefd674c94f10a843cd0e531a1a25c89eac225076

    Thurin identity:
    https://thurin.id/eth/0xB8A0f79E6d64c948E9F5ea32aD93647777915EBa

That previous index-0 result is shown as evidence of the already-completed
Mainnet publication step. It must not be presented as the output of the live
index-1 demonstration.

The runner does not automatically open these reference URLs.

For the recorded demonstration, the browser is prepared separately with the
previous Etherscan transaction and Thurin identity page already open. After the
runner reaches `DEMO COMPLETE`, the presenter takes over the browser and shows
those existing index-0 Mainnet results.


---


# What this demo proves

At a high level:

    physical Keycard
          │
          ├── OpenPGP key
          │      │
          │      └── signs:
          │          "I control the Ethereum address: 0x..."
          │
          └── Ethereum key
                 │
                 └── signs:
                     Thurin EIP-712 authorization

The host verifies both sides:

    OpenPGP signature
          │
          └── Thurin Identity Kit verifies
              PGP identity ↔ Ethereum address
                          │
                          ▼
                 OPENPGP → EVM BINDING: PASS

    EIP-712 signature
          │
          └── viem recovers Ethereum signer
                          │
                          ▼
                 recovered address == runtime address
                          │
                          ▼
                 EIP-712 OWNER SIGNATURE: PASS

This is important because the two signatures are produced by different keys
with different purposes.

The claim is not:

    "one key is being reused for PGP and Ethereum"

Instead:

    OpenPGP key                  Ethereum key
         │                            │
         └──── same Keycard hardware ─┘
                       │
                       ▼
              cryptographic binding

---

# Components

## Keycard Shell

Keycard Shell is the user-facing hardware device in this demo.

The Shell provides:

- the trusted display;
- physical user review and approval;
- QR scanning;
- QR response display;
- orchestration of signing operations; and
- communication with the inserted Keycard.

The host computer prepares requests, but signing decisions are reviewed on the
Shell.

The computer does not receive either private key.

---

## Keycard

The Keycard is the smart card holding the cryptographic key material.

In this demo it provides two separate signing identities.

### OpenPGP identity

The OpenPGP private key is derived using Shell-owned policy and uses a
non-exportable Keycard derivation path.

The host does not supply the OpenPGP private key.

The host does not supply the OpenPGP public key.

The host does not select the OpenPGP derivation path.

The public OpenPGP certificate is safe to export and is used by the host for
verification.

### Ethereum identity

The Ethereum account uses the normal Ethereum derivation hierarchy.

For the demonstrated account the full signing path resolves to:

    m/44'/60'/0'/0/0

The exact runtime account metadata is acquired from the Shell rather than being
typed into the demo script.

---

# OpenPGP

OpenPGP is the public-key format and signature system used for the identity side
of this demo.

The current Shell implementation supports two initial operations:

    CREATE_IDENTITY
    SIGN_MESSAGE

## CREATE_IDENTITY

CREATE_IDENTITY establishes the public OpenPGP identity corresponding to the
hardware-backed private key.

The Shell constructs and returns a public certificate containing:

- the primary public key;
- the User ID;
- the User ID self-certification signature.

The resulting certificate can be verified using standard OpenPGP tooling such
as GnuPG.

The certificate is public data.

The private OpenPGP key stays on the Keycard.

The Thurin demo assumes CREATE_IDENTITY has already been completed and that the
resulting certificate is available locally.

Default location:

    /tmp/openpgp-created-identity.pgp

A different certificate may be supplied using:

    OPENPGP_CERT=/path/to/certificate.pgp

The runner extracts the key creation time and fingerprint from the actual
certificate at runtime.

They are not separately hard-coded.

## SIGN_MESSAGE

SIGN_MESSAGE asks the OpenPGP key to produce a detached canonical-text
signature over a reviewed message.

For this demo the message is generated by Thurin and has the form:

    I control the Ethereum address: 0x...

The Shell displays the content before signing.

The returned object is an OpenPGP v4 Signature packet using:

- canonical-text signature type;
- ECDSA;
- SHA-256.

The host validates the structure before using it.

The current protocol limits signed text to 104 bytes and accepts printable
ASCII with LF or CRLF line endings.

---

# QR transport and BC-UR

The Shell is used as an air-gapped signer.

Requests and responses cross the computer/device boundary visually using QR
codes rather than USB or Bluetooth signing APIs.

There are two related QR protocols in this demo.

## OpenPGP QR transport

The current OpenPGP protocol is experimental and intentionally small.

OpenPGP requests are encoded as CBOR and wrapped using:

    UR:BYTES

The same generic UR type is used to return OpenPGP results.

For example:

    host
      │
      ├── CBOR SIGN_MESSAGE request
      │
      └── UR:BYTES QR
              │
              ▼
         Keycard Shell
              │
              ├── review
              ├── sign
              │
              └── UR:BYTES response
                         │
                         ▼
                        host

The host tooling uses BC-UR support from @ngraveio/bc-ur.

---

# ERC-4527

The source code historically calls this portion of the flow "EIP4527."

The standard is currently published as ERC-4527.

ERC-4527 defines QR communication between an online/watch-only wallet and an
offline signer.

It uses typed UR objects for Ethereum wallet operations.

The important types in this demo are:

    crypto-hdkey
    eth-sign-request
    eth-signature

## crypto-hdkey

The Shell exports public Ethereum account metadata through a crypto-hdkey QR.

The host decodes:

- the account origin;
- child derivation information when present; and
- the source fingerprint.

For the normal Shell Ethereum account:

    origin:
        m/44'/60'/0'

The normal address screen then uses:

    change = 0
    address index = 0

giving:

    m/44'/60'/0'/0/0

The source fingerprint is also retained so that the later Ethereum signing
request targets the correct card/account context.

No private Ethereum material is included in crypto-hdkey.

## eth-sign-request

After Thurin produces EIP-712 typed data, the host wraps that data into an
ERC-4527 eth-sign-request.

The request includes:

- EIP-712 typed data;
- derivation path;
- source fingerprint;
- request ID;
- chain ID;
- Ethereum address;
- origin/application label.

If the payload is too large for one QR code, BC-UR splits it into animated QR
fragments.

The Shell's normal Ethereum QR scanner reads this request.

This is important: the Ethereum half of the demo uses the Shell's standard
Ethereum signing path rather than a special OpenPGP signing path.

## eth-signature

After review and approval, the Shell returns an ERC-4527 eth-signature QR.

The host decodes it and checks that its request ID corresponds to the original
request.

The Ethereum signature is 65 bytes:

    r || s || v

---

# EIP-712

EIP-712 defines a structured Ethereum signing format.

Instead of asking a signer to approve an opaque arbitrary byte string, the
payload has explicit:

- types;
- domain;
- primary type;
- message fields.

Thurin uses EIP-712 for the Ethereum-owner authorization in this demo.

The sign-out data contains a typed request describing the Thurin attestation
authorization.

The host converts this typed data into ERC-4527 eth-sign-request QR frames.

## EIP712Domain compatibility

The Thurin sign-out payload used during development did not always include an
explicit EIP712Domain entry inside the `types` object.

The Shell Ethereum parser expects the domain structure to be declared.

`generate-thurin-eth-request.mjs` therefore adds the standard domain declaration
when it is absent.

The demonstrated fields are:

    name
    version
    chainId
    verifyingContract

This compatibility transformation was exercised in the successful hardware
flow.

The resulting hardware signature was later recovered against the Thurin typed
data and matched the runtime Ethereum owner address.

---

# Thurin

Thurin is the identity system used as the external integration in this demo.

The purpose of using Thurin is not merely to test an isolated OpenPGP signature.

It gives the signature a real application:

    bind an OpenPGP identity to an Ethereum identity

Thurin provides:

- the exact ownership statement to sign;
- Identity Kit verification;
- Ethereum authorization data;
- the identity/attestation protocol; and
- the final on-chain identity record.

The demo uses:

    @thurinlabs/identity-kit 2.2.1
    @thurinlabs/thurin       0.13.8

---

# What is an attestation?

An attestation is a cryptographically authorized claim.

In this flow the relevant claim connects:

    OpenPGP identity
           ↕
    Ethereum address

The OpenPGP side proves that the PGP identity signed the ownership statement.

The Ethereum side proves that the owner of the claimed Ethereum address
authorized the corresponding Thurin action.

Thurin can then represent that relationship as an identity claim that can be
looked up and verified independently.

The attestation is therefore not merely:

    "here is a PGP signature"

and not merely:

    "here is an Ethereum signature"

It is the combination of the two proofs around one identity relationship.

---

# Host-side tools

## GnuPG

GnuPG is used to inspect the public OpenPGP certificate and derive:

- primary-key creation time;
- fingerprint.

It is also useful for independent OpenPGP inspection and verification.

GnuPG never receives the private Keycard key.

## Node.js

The host protocol and decoder utilities are JavaScript ES modules.

Node.js runs:

- OpenPGP request construction;
- OpenPGP response decoding;
- ERC-4527 metadata decoding;
- ERC-4527 Ethereum request generation;
- ERC-4527 signature decoding;
- Thurin Identity Kit verification.

## cbor2

The OpenPGP request protocol uses CBOR maps.

`cbor2` is used to construct and self-check those messages.

## @ngraveio/bc-ur

This package handles generic BC-UR encoding and decoding.

It is used by both the OpenPGP QR transport and Ethereum QR integration.

## @keystonehq/bc-ur-registry

This package provides registry objects such as crypto-hdkey.

The demo uses it to decode the runtime Ethereum account metadata exported by
the Shell.

## @keystonehq/bc-ur-registry-eth

This package provides Ethereum-specific UR registry types.

The demo uses:

    EthSignRequest
    ETHSignature

for ERC-4527 request/response transport.

## FFmpeg

FFmpeg captures camera frames from the Mac.

Current camera integration uses macOS AVFoundation.

## ZBar

`zbarimg` scans the captured frames for QR payloads.

The runner now scans frames while capture is still active.

As soon as enough UR fragments have been reconstructed it prints:

    QR capture complete (...) — you can lower the Shell now.

This avoids requiring the user to hold a response QR in front of the camera
while unrelated post-processing continues.

## qrencode

`qrencode` turns UR fragments into QR images.

It is used when creating requests for the Shell.

## viem

The demo pins:

    viem 2.56.7

The runner uses `recoverTypedDataAddress()` after the hardware EIP-712
signature is returned.

It reconstructs the Ethereum address that produced the signature.

That recovered address must equal the address captured from the physical
Keycard at the beginning of the run.

---

# Demo files

## Existing OpenPGP identity tools

    generate-request.mjs
        Builds CREATE_IDENTITY request.

    decode-response.mjs
        Decodes CREATE_IDENTITY response and recovers the certificate.

## Message-signing tools

    generate-sign-message-request.mjs
        Builds the OpenPGP SIGN_MESSAGE request.

    decode-sign-message-response.mjs
        Decodes and validates the returned OpenPGP Signature packet.

## Ethereum account tools

    capture-evm-address.sh
        Captures the normal Ethereum address QR.

    decode-eip4527-account.mjs
        Decodes the crypto-hdkey account metadata.

## Ethereum signing tools

    generate-thurin-eth-request.mjs
        Converts Thurin EIP-712 typed data into an ERC-4527
        eth-sign-request.

    decode-eth-signature.mjs
        Decodes ERC-4527 eth-signature output and verifies the request ID.

## QR completeness helper

    check-ur-complete.mjs
        Tests whether the currently captured UR fragments reconstruct a
        complete object.

        This allows the runner to stop camera capture as soon as enough data
        has been seen.

## Full runner

    run-thurin-keycard-e2e.sh

This orchestrates the complete demo.

---

# Complete demo flow

## Step 0 — Existing OpenPGP identity

Before this demo begins, CREATE_IDENTITY has already run.

Expected public certificate:

    /tmp/openpgp-created-identity.pgp

The runner reads this certificate and derives:

    OpenPGP fingerprint
    OpenPGP key creation time

The corresponding private key remains on the Keycard.

---

## Step 1 — Capture runtime Ethereum address

On Shell:

    Ethereum
      → address index 0

Display the address QR.

The runner invokes:

    capture-evm-address.sh

The computer camera scans the QR and extracts an address matching:

    0x + 40 hexadecimal characters

Expected checkpoint:

    ADDRESS QR: PASS

This address becomes `OWNER`.

Nothing is manually typed into the runner.

---

## Step 2 — Capture runtime ERC-4527 account metadata

On Shell:

    Connect
      → Ethereum

Shell displays a crypto-hdkey QR.

The runner captures and decodes it using:

    decode-eip4527-account.mjs

The decoder obtains:

    origin path
    child derivation information
    source fingerprint

For the standard account, the resulting signing path is:

    m/44'/60'/0'/0/0

Expected checkpoint:

    EIP4527 METADATA: PASS

---

## Step 3 — Ask Thurin for the statement

The runner asks Thurin to generate the ownership statement for the runtime
Ethereum address.

The result is:

    I control the Ethereum address: 0x...

This exact string becomes the OpenPGP message.

The host does not invent a different statement.

---

## Step 4 — Sign the statement with OpenPGP

The runner calls:

    generate-sign-message-request.mjs

Inputs:

    exact Thurin statement
    OpenPGP key creation time
    current signature creation time

The generator:

1. validates the message;
2. constructs the protocol CBOR;
3. wraps it as UR:BYTES;
4. self-decodes the UR;
5. checks the recovered CBOR;
6. writes the request QR.

Shell scans the QR.

The user reviews the actual message and metadata on the trusted device.

The OpenPGP private key signs only after approval.

Shell displays the response QR.

The runner scans it and stops capture as soon as a complete UR has been
reconstructed.

Expected feedback:

    Camera active — hold the response QR in view...
    QR capture complete (...) — you can lower the Shell now.

The response decoder verifies that the returned object is:

    OpenPGP packet tag:       2
    version:                  4
    signature type:           canonical text
    public-key algorithm:     ECDSA
    hash algorithm:           SHA-256

---

## Step 5 — Verify OpenPGP → Ethereum binding

The host now has:

    public OpenPGP certificate
    detached OpenPGP signature
    OpenPGP fingerprint
    runtime Ethereum address

These are passed to Thurin Identity Kit.

Identity Kit verifies that the OpenPGP signature represents the expected claim
about the Ethereum address.

Expected checkpoint:

    OPENPGP → EVM BINDING: PASS

At this point the PGP half of the relationship has been proven.

---

## Step 6 — Ask Thurin for Ethereum authorization

The runner asks Thurin for an air-gapped authorization package using:

    --authorize
    --sign-out

Thurin writes a sign-out JSON file.

This contains the EIP-712 typed data that the Ethereum owner must authorize.

Expected checkpoint:

    THURIN SIGN-OUT: PASS

---

## Step 7 — Build the ERC-4527 eth-sign-request

The runner calls:

    generate-thurin-eth-request.mjs

Inputs:

    Thurin sign-out JSON
    runtime derivation path
    runtime source fingerprint
    runtime Ethereum owner

The generator:

1. reads the EIP-712 typed data;
2. adds EIP712Domain type information when necessary;
3. creates a random request ID;
4. creates an EthSignRequest;
5. encodes it as `eth-sign-request`;
6. splits it into animated UR frames.

The runner renders those frames to QR images and creates an animated request
for the Shell.

---

## Step 8 — Sign the Ethereum authorization

On Shell:

    open normal Ethereum QR scanner

Scan the animated eth-sign-request.

Shell parses the request and displays the EIP-712 information for review.

After approval, the Ethereum key on the same Keycard signs the request.

Shell returns:

    eth-signature

The runner captures the QR and decodes it with:

    decode-eth-signature.mjs

The decoder checks:

- UR type;
- response request ID;
- 65-byte Ethereum signature length.

---

## Step 9 — Recover and verify the Ethereum signer

The runner uses viem:

    recoverTypedDataAddress()

Inputs:

    original Thurin typed data
    hardware-generated Ethereum signature

It prints:

    expected:  0x...
    recovered: 0x...

The two addresses must match.

Expected checkpoint:

    EIP-712 OWNER SIGNATURE: PASS

This proves that the EIP-712 authorization was produced by the runtime
Ethereum account discovered from the physical Keycard.

---

## Step 10 — Finish Thurin authorization

The runner provides the hardware-generated Ethereum signature to:

    thurin authorize finish

This completes the authorization handoff to Thurin.

Depending on the Thurin publication/relay path in use, the CLI may perform the
next publication action directly or provide a handoff for completing it.

The important hardware checks have already completed before this point:

    OPENPGP → EVM BINDING: PASS
    EIP-712 OWNER SIGNATURE: PASS

A successful end-to-end run can then result in an Ethereum mainnet Thurin
identity attestation.

---

# Security properties

## Private keys remain on hardware

The host receives:

- public certificate;
- Ethereum public account metadata;
- detached OpenPGP signature;
- Ethereum signature.

It does not receive either private key.

## Distinct keys, explicit relationship

The OpenPGP and Ethereum keys are distinct.

The binding exists because both sides sign semantically related data.

## Runtime Ethereum discovery

The owner address is captured from the physical Shell.

The ERC-4527 account origin and source fingerprint are also acquired at runtime.

The demo therefore avoids separately configuring a second Ethereum identity on
the host.

## Trusted review

The host constructs requests.

The Shell is responsible for displaying the relevant content and requiring
physical user confirmation before invoking the Keycard signing operation.

## Local verification before publication

The host verifies both signature relationships before the final Thurin finish
step.

A failure in either verification aborts the runner.

---

# Requirements

Current known-good environment:

- macOS
- physical Keycard Shell
- Keycard containing the tested keys
- Node.js 20 or newer
- npm
- GnuPG
- FFmpeg
- ZBar
- qrencode

The current camera scripts use AVFoundation and are therefore macOS-specific.

Linux and Windows camera integration can be added separately without changing
the cryptographic protocol.

---

# Install dependencies

From:

    experiments/shell-port/host-e2e

run:

    npm ci

From:

    experiments/thurin

run:

    npm ci

The Thurin experiment currently uses:

    @thurinlabs/identity-kit 2.2.1
    @thurinlabs/thurin       0.13.8
    viem                     2.56.7

---

# Run the complete demo

From the repository root:

    chmod +x \
      experiments/shell-port/host-e2e/capture-evm-address.sh \
      experiments/shell-port/host-e2e/run-thurin-keycard-e2e.sh

Then:

    experiments/shell-port/host-e2e/run-thurin-keycard-e2e.sh

If the Mac camera is not device 0:

    CAMERA_DEVICE=<index> \
      experiments/shell-port/host-e2e/run-thurin-keycard-e2e.sh

To use a different public OpenPGP certificate:

    OPENPGP_CERT=/path/to/key.pgp \
      experiments/shell-port/host-e2e/run-thurin-keycard-e2e.sh

---

# Expected checkpoints

A successful run should pass these checkpoints in order:

    ADDRESS QR: PASS

    EIP4527 METADATA: PASS

    REQUEST SELF-CHECK: PASS

    RESPONSE STRUCTURE CHECK: PASS

    OPENPGP → EVM BINDING: PASS

    THURIN SIGN-OUT: PASS

    EIP-712 OWNER SIGNATURE: PASS

    FULL HARDWARE AUTHORIZATION COMPLETE

---

# What the video should emphasize

The core story is not the shell script.

The core story is:

    one physical Keycard
          │
          ├── hardware-backed OpenPGP identity
          │
          └── hardware-backed Ethereum identity
                     │
                     ▼
        both participate in one real application flow

The most important moments to show visibly are:

1. the Ethereum address coming directly from Shell;
2. the OpenPGP statement displayed on Shell;
3. `OPENPGP → EVM BINDING: PASS`;
4. the standard Ethereum QR scanner reading the EIP-712 request;
5. the hardware Ethereum signature being returned;
6. the recovered address matching the runtime address;
7. `EIP-712 OWNER SIGNATURE: PASS`;
8. the resulting Thurin identity / mainnet attestation.

A concise explanation for the video:

> The OpenPGP key and Ethereum key are different keys, but both live on the
> same physical Keycard. The PGP key signs the identity binding. The Ethereum
> key authorizes the Ethereum side. The computer verifies both proofs, but
> neither private key ever leaves the card.

---

# Current scope

This is integration and demo tooling.

It is not yet the final consumer UX.

The expected evolution is:

    CLI
      ↓
    browser / no-CLI flow
      ↓
    software wallet handles more transport and orchestration
      ↓
    reusable Keycard OpenPGP signing across many applications

The current scripts are valuable because they establish the complete protocol
and verification path before that UX is abstracted behind a browser or wallet.

---

# Runner documentation

For the exact executable flow, recording-safe terminal behavior, video
redaction policy, and expected checkpoints, see:

    RUNNER_README.md

---

# Related files

OpenPGP host E2E:

    README.md

Shell/OpenPGP roadmap:

    ../ROADMAP.md

Security review:

    ../SECURITY_REVIEW.md

This document describes the higher-level Thurin integration built on top of
those primitives.

---

# Standards and projects involved

- Keycard Shell
- Keycard
- OpenPGP
- BC-UR
- ERC-4527 QR Code transmission protocol for wallets
- EIP-712 typed structured data signing
- Ethereum
- Thurin
- Thurin Identity Kit
- GnuPG
- viem
- FFmpeg
- ZBar
- qrencode
