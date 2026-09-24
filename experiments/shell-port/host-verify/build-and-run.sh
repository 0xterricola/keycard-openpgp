#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 <keycard-shell-dir> <bundle.gpg>" >&2
    exit 2
fi

SHELL_DIR="$1"
BUNDLE="$2"
SHELL_APP="$SHELL_DIR/app"

if [ ! -f "$SHELL_APP/openpgp/openpgp_v4.c" ]; then
    echo "error: OpenPGP Shell sources not found under $SHELL_APP" >&2
    exit 1
fi

if [ ! -f "$BUNDLE" ]; then
    echo "error: bundle not found: $BUNDLE" >&2
    exit 1
fi

OPENSSL_PREFIX="$(brew --prefix openssl)"

cc -Wall -Wextra -Wno-deprecated-declarations \
    -I. -I"$SHELL_APP" \
    -I"$OPENSSL_PREFIX/include" \
    verify_test.c shims.c \
    "$SHELL_APP/openpgp/openpgp_packet.c" \
    "$SHELL_APP/openpgp/openpgp_v4.c" \
    -L"$OPENSSL_PREFIX/lib" -lcrypto \
    -o ./verify_test

./verify_test "$BUNDLE"
