# Keycard Shell OpenPGP Reference Client

Status: Experimental.

A minimal Node.js command-line interface demonstrating how host applications
can communicate with Keycard Shell's OpenPGP functionality without using
Thurin Labs services or libraries.

This is a repository-based reference implementation, not a standalone
installable package.

## Supported operations

- CREATE_IDENTITY: Generate an OpenPGP identity request.
- SIGN_MESSAGE: Generate a canonical-text signing request.
- Decode OpenPGP certificate responses.
- Decode detached OpenPGP signature responses.
- Verify detached signatures using GnuPG.

## Requirements

- Node.js 20+
- GnuPG
- qrencode
- ZBar for decoding QR images
- Existing host tooling dependencies

Install the Node.js dependencies:

    cd experiments/shell-port/host-e2e
    npm ci
    cd ../../..

On macOS, external utilities can be installed with:

    brew install node gnupg qrencode zbar

## Usage

Run commands from the repository root.

### Create an identity request

    node reference-client/src/cli.mjs create-identity \
      --uid "Keycard Test <keycard@example.com>"

Produces:

    /tmp/openpgp-create-identity.ur
    /tmp/openpgp-create-identity.png

Scan the QR request with Keycard Shell and approve the identity creation.

### Decode an identity response

    node reference-client/src/cli.mjs decode-identity \
      --input response.ur

Produces:

    /tmp/openpgp-created-identity.pgp

### Generate a signing request

    node reference-client/src/cli.mjs sign-message \
      --cert certificate.pgp \
      --file message.txt

The client extracts the key creation timestamp from the certificate.

Produces:

    /tmp/openpgp-sign-message.ur
    /tmp/openpgp-sign-message.png
    /tmp/openpgp-sign-message.txt

Scan the QR request with Shell, review the message, and approve signing.

### Decode a signature response

    node reference-client/src/cli.mjs decode-signature \
      --input response.ur

Produces:

    /tmp/openpgp-sign-message.sig

### Verify a detached signature

    node reference-client/src/cli.mjs verify \
      --cert certificate.pgp \
      --file message.txt \
      --signature message.sig

Verification uses an isolated temporary GnuPG home.

## Automated tests

Run:

    bash reference-client/tests/test-verify.sh
    bash reference-client/tests/test-decode.sh

The tests use existing hardware-generated OpenPGP artifacts.

The QR decoding tests reconstruct synthetic UR:BYTES responses from
those artifacts and verify byte-for-byte recovery.

They do not require physical hardware.

## Security considerations

- Shell selects the hardware-backed key derivation path.
- Private keys are not exported through this protocol.
- Shell displays signing information for physical approval.
- The host provides timestamps and message content.
- The host must verify that the returned signature matches the intended
  certificate and message.
- Cryptographic signature validity does not establish identity trust.
- GnuPG verification uses a temporary keyring.
- Current request generators write output to fixed /tmp paths.
- This experimental client has not undergone an independent security audit.

## Limitations

- The existing JavaScript signing request generator limits messages to
  104 bytes; current firmware allows up to 255 bytes.
- Only printable ASCII and supported line endings are accepted.
- Arbitrary file signing is not supported by the current SIGN_MESSAGE API.
- QR capture remains a manual or external-tool workflow.
- Linux and Windows have not been independently validated end-to-end.
- The client depends on scripts elsewhere in this repository.
- Thunderbird and Git commit signing integrations are not implemented.
- secp256k1 support varies across OpenPGP implementations.

## Protocol documentation

See [OpenPGP Protocol v1](../docs/OPENPGP_PROTOCOL.md).

## Roadmap

- Package a reusable client independently of the experimental host scripts.
- Improve output path handling and QR capture.
- Explore release signing, Git commit signing, and Thunderbird integration.
