# Keycard Shell OpenPGP host verification

Host-side test harness for the OpenPGP primitives being added to
Keycard Shell.

The harness compiles the real Shell OpenPGP sources while replacing
Shell's hardware-dependent SHA and ECDSA interfaces with OpenSSL-backed
host shims.

It verifies that the known Keycard-generated OpenPGP test bundle:

- parses as a primary public key, User ID, and self-certification
- produces fingerprint `A1265C689EC7C60018C0AFB023A92D87D63F7E7C`
- contains User ID `keycard-test`
- has a valid secp256k1 ECDSA UID self-certification

The OpenSSL shims are test-only. Production Shell firmware continues to
use Shell's native crypto implementation.

## Run

Requires macOS, Homebrew OpenSSL, and a Keycard Shell checkout containing
the OpenPGP sources.

    ./build-and-run.sh \
      ~/Developer/keycard-shell \
      fixtures/keycard-test.gpg

The generated `verify_test` executable is ignored by Git.
