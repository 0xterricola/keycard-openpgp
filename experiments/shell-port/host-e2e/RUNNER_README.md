# Keycard Shell → OpenPGP → Thurin → Ethereum Runner

This document describes the executable end-to-end demo runner:

    run-thurin-keycard-e2e.sh

For the deeper explanation of the cryptography, protocols, standards, and
architecture, see:

    THURIN_E2E_DEMO.md

The runner is designed both as an integration test and as the executable flow
used for the public demo video.

No individual contributor or maintainer names are required by this
documentation. Project participants should be referred to by role or team,
for example:

    Keycard team
    Keycard Shell maintainer
    Thurin Labs team
    Thurin maintainer

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


# What the runner demonstrates

One physical Keycard participates in two different cryptographic roles:

    Keycard
      |
      +-- OpenPGP key
      |     |
      |     +-- signs an Ethereum ownership statement
      |
      +-- Ethereum key
            |
            +-- signs a Thurin EIP-712 authorization

The two keys are separate.

The relationship between them is established cryptographically through the
application flow.

Neither private key is exported to the Mac.

---

# Main command

From the repository root:

    CAMERA_DEVICE=0 \
      experiments/shell-port/host-e2e/run-thurin-keycard-e2e.sh

To record a log:

    CAMERA_DEVICE=0 \
      experiments/shell-port/host-e2e/run-thurin-keycard-e2e.sh \
      2>&1 | tee /tmp/keycard-thurin-rehearsal.log

The default output mode is intended to be safe for screen recording.

---

# Debug mode

Additional transport-level identifiers can be shown with:

    DEMO_VERBOSE=1 \
    CAMERA_DEVICE=0 \
      experiments/shell-port/host-e2e/run-thurin-keycard-e2e.sh

Do not use verbose mode for the public recording unless the resulting terminal
output has been reviewed first.

Normal mode intentionally suppresses unnecessary values such as:

    full OpenPGP request CBOR
    full OpenPGP request UR
    ERC-4527 source fingerprint
    Ethereum request UUID

These values still exist internally when the protocol requires them.

They simply are not printed to the recording terminal.

---

# Public OpenPGP certificate

The runner expects the stable public demo certificate at:

    fixtures/openpgp-created-identity.pgp

This certificate is public information.

It contains no OpenPGP private key material.

The corresponding private key remains on the Keycard.

The demo certificate uses the intentionally generic demo identity:

    Keycard Test <keycard@example.com>

Because the certificate is public and is part of the reproducible demo
fixture, it is intended to be committed to Git.

A different certificate can be supplied with:

    OPENPGP_CERT=/path/to/public-certificate.pgp \
      experiments/shell-port/host-e2e/run-thurin-keycard-e2e.sh

The runner derives the OpenPGP key creation time and fingerprint from the
certificate at runtime.

They do not need to be configured separately.

---

# Recording-safe terminal policy

The normal runner intentionally distinguishes between public demonstration
information and unnecessary raw transport artifacts.

The following information is expected to remain visible in the published
terminal recording:

    Ethereum public address
    Ethereum derivation path
    network / chain information
    Thurin ownership statement
    OpenPGP fingerprint
    signature byte lengths
    OpenPGP packet metadata
    recovered Ethereum address
    verification results
    PASS / FAIL checkpoints

The following information is intentionally hidden or summarized by default:

    raw OpenPGP request CBOR
    raw OpenPGP UR payload
    ERC-4527 source fingerprint
    Ethereum request UUID
    raw Ethereum signature hex
    raw QR response fragments
    full authorization JSON

The raw 65-byte Ethereum signature is still required internally.

`decode-eth-signature.mjs` writes the signature to stdout, but the runner
redirects that output directly into a per-run temporary file.

The raw signature is never stored in a shell variable and is never printed to
the recording terminal.

The same temporary file is used for local viem verification and for
`thurin authorize finish --signature-file`.

The runner cleanup removes the file when the run ends.

---

# Physical-device video policy

Some information never appears in the terminal but may appear physically on
the Keycard Shell.

For the published video, place an opaque cover over:

    crypto-hdkey QR responses
    OpenPGP signature response QR
    Ethereum eth-signature response QR
    PIN entry
    recovery material
    private-key material

An opaque rectangle is preferred over a light blur for QR responses.

The following can normally remain visible:

    Ethereum public address QR
    request QR going into the Shell
    transaction / EIP-712 review screens
    OpenPGP statement review
    public OpenPGP fingerprint
    final public transaction hash
    final public Thurin identity page

Always review the final rendered video before publication.

---

# Runner sequence

## 1. Runtime Ethereum address

On the Shell:

    Ethereum
      -> address index 0

The runner starts the camera and scans the displayed address QR.

The resulting public address becomes the runtime `OWNER`.

Expected checkpoint:

    ADDRESS QR: PASS

---

## 2. ERC-4527 account metadata

On the Shell:

    Connect
      -> Ethereum

The Shell displays a `crypto-hdkey` QR.

The host decodes:

    Ethereum account origin
    signing derivation path
    source fingerprint

The standard tested signing path is:

    m/44'/60'/0'/0/0

The real source fingerprint remains available internally but is summarized in
normal recording mode.

Expected checkpoint:

    EIP4527 METADATA: PASS

---

## 3. Thurin ownership statement

The runner requests the exact statement for the runtime Ethereum owner.

The statement has the form:

    I control the Ethereum address: 0x...

This exact statement becomes the OpenPGP message.

---

## 4. OpenPGP SIGN_MESSAGE

The host constructs the OpenPGP signing request.

In normal recording mode the terminal shows metadata and validation results
without dumping the raw CBOR or full UR payload.

The request QR is displayed on the Mac.

The Shell scans it.

The user reviews the message on the Shell and approves the OpenPGP signature.

The Shell displays the response QR.

The Mac camera scans it live.

Expected camera feedback:

    Camera active
    QR detected
    QR capture complete
    you can lower the Shell now

The response decoder verifies:

    OpenPGP packet tag 2
    version 4
    canonical-text signature
    ECDSA
    SHA-256

Expected checkpoint:

    RESPONSE STRUCTURE CHECK: PASS

---

## 5. OpenPGP to Ethereum verification

The runner gives Thurin Identity Kit:

    public OpenPGP certificate
    detached OpenPGP signature
    OpenPGP fingerprint
    runtime Ethereum address

The host verifies the cryptographic binding locally.

Expected output:

    identity-kit verification: PASS
    OPENPGP -> EVM BINDING: PASS

---

## 6. Thurin air-gap authorization

The runner asks Thurin to produce an offline Ethereum authorization package.

The resulting sign-out data contains EIP-712 typed structured data.

The sign-out file is kept in the per-run temporary directory.

It is not intended to be dumped to the recording terminal.

Expected checkpoint:

    THURIN SIGN-OUT: PASS

---

## 7. ERC-4527 Ethereum signing request

The runner converts the Thurin EIP-712 authorization into:

    eth-sign-request

The request includes the runtime:

    derivation path
    source fingerprint
    Ethereum owner
    chain ID
    request ID

The source fingerprint and request UUID are summarized rather than displayed
in normal recording mode.

The request is encoded into animated BC-UR QR frames.

---

## 8. Ethereum hardware signature

The Shell scans the normal Ethereum QR request.

The user reviews and approves the EIP-712 authorization.

The Shell returns:

    eth-signature

The Mac captures it.

The full Ethereum signature is written directly to a per-run temporary file
and is not printed.

The signature file is used for local signer recovery and then passed to Thurin
with `--signature-file`.

It is safe for the terminal to show structural information such as:

    Ethereum signature: 65 bytes
    v: 0x1b

The physical response QR should still be covered in the published video.

---

## 9. Recover Ethereum signer

The runner uses viem to recover the signer from:

    original EIP-712 typed data
    hardware-produced Ethereum signature

It compares:

    expected runtime address
    recovered signer address

These are public Ethereum addresses and may remain visible.

Expected checkpoint:

    EIP-712 OWNER SIGNATURE: PASS

---

## 10. Finish Thurin authorization

The verified hardware Ethereum signature is passed to:

    thurin authorize finish

At this point both hardware-backed proofs have already been checked locally:

    OPENPGP -> EVM BINDING: PASS
    EIP-712 OWNER SIGNATURE: PASS

The final Thurin publication or relay behavior may then complete the identity
operation.

---

# QR capture behavior

The runner scans QR responses while FFmpeg is still capturing camera frames.

It does not intentionally wait for the full capture timeout when enough UR
fragments have already been collected.

Typical output:

    Camera active
    QR detected (1 fragment)
    QR detected (2 fragments)
    QR capture complete
    you can lower the Shell now

This is especially useful during video recording because the user knows
exactly when the device no longer needs to remain in front of the Mac camera.

---

# Host tools

The runner currently uses:

    Bash
    Node.js
    GnuPG
    FFmpeg
    ZBar
    qrencode
    BC-UR
    ERC-4527 registry libraries
    Thurin CLI
    Thurin Identity Kit
    viem

Detailed descriptions of these components are in:

    THURIN_E2E_DEMO.md

---

# Important files

    run-thurin-keycard-e2e.sh
        Complete orchestration.

    capture-evm-address.sh
        Live Ethereum-address QR capture.

    check-ur-complete.mjs
        Determines when enough UR fragments have been captured.

    decode-eip4527-account.mjs
        Decodes runtime Ethereum public account metadata.

    generate-sign-message-request.mjs
        Creates OpenPGP SIGN_MESSAGE request.

    decode-sign-message-response.mjs
        Parses the OpenPGP signature response.

    generate-thurin-eth-request.mjs
        Converts Thurin EIP-712 data into eth-sign-request.

    decode-eth-signature.mjs
        Decodes the hardware Ethereum signature response.

    fixtures/openpgp-created-identity.pgp
        Stable public OpenPGP demo certificate.

    THURIN_E2E_DEMO.md
        Full technical architecture and protocol explanation.

---

# Temporary data

Per-run camera frames and transport artifacts belong in temporary storage.

Examples include:

    captured QR frames
    reconstructed UR fragments
    Thurin sign-out JSON
    request IDs
    detached response artifacts

The stable public OpenPGP certificate is not temporary.

It belongs in the committed fixtures directory.

No private key should ever be written by this runner.

---

# Expected checkpoints

A successful demonstration should show:

    ADDRESS QR: PASS

    EIP4527 METADATA: PASS

    REQUEST SELF-CHECK: PASS

    RESPONSE STRUCTURE CHECK: PASS

    identity-kit verification: PASS

    OPENPGP -> EVM BINDING: PASS

    THURIN SIGN-OUT: PASS

    EIP-712 OWNER SIGNATURE: PASS

    FULL HARDWARE AUTHORIZATION COMPLETE

---

# Pre-recording checklist

Before recording:

    use normal mode, not DEMO_VERBOSE=1

    verify the public OpenPGP fixture fingerprint

    confirm the Shell firmware version

    clear unrelated terminal scrollback

    close windows containing credentials

    disable notifications

    make sure no seed phrase or PIN is visible

    run one complete rehearsal

    confirm the terminal never prints raw Ethereum signature hex

    confirm the terminal never prints raw UR response fragments

After recording:

    cover sensitive response QRs with opaque blocks

    review every Shell QR shown on screen

    review terminal output frame by frame around signing operations

    scan the final rendered video with a QR reader

    publish only after the final rendered copy passes review

---

# Security model

The host is an orchestrator and verifier.

The Keycard remains the signing authority.

The Mac may know:

    public keys
    public addresses
    public certificates
    messages being signed
    signatures after signing

The Mac must not learn:

    OpenPGP private key
    Ethereum private key
    seed phrase
    PIN
    recovery secret

The presence of a signature on the Mac is expected.

The presence of a private key is not.

---

# Documentation naming policy

Public documentation for this work should avoid personal names of project
maintainers and team members.

Use organizational or role-based references instead:

    Keycard team
    Keycard Shell maintainer
    Thurin Labs team
    Thurin maintainer

This keeps the documentation focused on the software and protocol rather than
individual contributors.

---

# Reading the completed Mainnet reference

The live recorded demonstration intentionally stops before publishing the
Ethereum index-1 authorization.

At the end of the demo, a previously completed Ethereum index-0 Mainnet
attestation is shown instead.

This section explains how to read that completed result.

## Thurin identity page

Reference address:

    0xB8A0f79E6d64c948E9F5ea32aD93647777915EBa

The Thurin identity page shows:

    Current key:
    07D8539F269583C160E333151763E15ED3E9286C

    PGP status:
    verified

    Published identity:
    Keycard Test <keycard@example.com>

    Key type:
    secp256k1

    Claim:
    #0 active

This is the human-readable representation of the binding.

The Ethereum address points to the same stable OpenPGP fingerprint used by the
hardware demo.

`pgp verified` means the published OpenPGP proof validates for that claim.

---

## Etherscan transaction

Completed reference transaction:

    0xddd7085ccc4f33865477eabaefd674c94f10a843cd0e531a1a25c89eac225076

Etherscan decodes the transaction input into six important fields.

### owner

    0xB8A0f79E6d64c948E9F5ea32aD93647777915EBa

This is the Ethereum address being associated with the OpenPGP identity.

### fingerprint

    07D8539F269583C160E333151763E15ED3E9286C

This is the stable OpenPGP fingerprint.

It is the same fingerprint shown:

    on the Keycard Shell
    in the host verification
    on the Thurin identity page
    in the Mainnet transaction

### signature

The `signature` argument contains the detached OpenPGP signature packet.

Its size is:

    119 bytes

This matches the hardware response shown during the live flow:

    Recovered signature: 119 bytes

This is the OpenPGP proof binding the ownership statement to the stable
OpenPGP key.

### key

The `key` argument contains the public OpenPGP certificate.

Its size is:

    239 bytes

This matches the stable public certificate used by this demo:

    public key packet
    User ID packet
    self-certification signature

The certificate contains public information only.

The OpenPGP private key remains on the Keycard.

### deadline

The completed transaction contains:

    1791739883

which corresponds to:

    2026-10-11 17:31:23 UTC

This is the expiration time associated with the Ethereum authorization.

### permission

The `permission` argument is the Ethereum authorization signature.

Its size is:

    65 bytes

This corresponds directly to the hardware output demonstrated by the runner:

    Ethereum signature: 65 bytes

The signature proves that the Ethereum key authorized the EIP-712 operation.

---

# Connecting the live demo to Mainnet

The most useful way to understand the completed transaction is to compare it
with the values produced during the hardware demonstration:

    Live hardware artifact             Mainnet transaction argument
    ----------------------------------------------------------------
    Ethereum owner                  -> owner
    OpenPGP fingerprint             -> fingerprint
    119-byte OpenPGP signature      -> signature
    239-byte public PGP certificate -> key
    authorization expiration        -> deadline
    65-byte Ethereum signature      -> permission

The Mainnet transaction therefore contains the public evidence produced by
both sides of the hardware-backed identity flow.

The OpenPGP key proves the identity statement.

The Ethereum key proves authorization by the Ethereum owner.

Neither private key is published or included in the transaction.

---

# Suggested recording walkthrough

After `DEMO COMPLETE`, switch from the live terminal to the previously opened
index-0 reference tabs.

On the Thurin identity page, point out:

    Ethereum address
    OpenPGP fingerprint
    PGP verified
    published UID
    secp256k1 key type
    active claim

Then switch to Etherscan and point out:

    transaction status: Success
    owner
    fingerprint
    signature
    key
    deadline
    permission

The important byte-size correspondence is:

    OpenPGP signature: 119 bytes
    OpenPGP certificate: 239 bytes
    Ethereum signature: 65 bytes

These are the same artifact types produced during the live hardware flow.

Do not present the index-0 transaction as the output of the live index-1
demonstration.

It is a previously completed Mainnet example showing what publication of the
same protocol flow looks like.
