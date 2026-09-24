#!/usr/bin/env python3
"""
Export a public key and sign a supplied digest on a Keycard.

Card state changes: none, except the PIN retry counter if the PIN is wrong.
Both export and sign use make_current=False.

Usage:
    python3 card_export_and_sign.py <index> <pairing_key_hex> <path> <digest_hex>
"""

import getpass
import sys

from keycard.keycard import KeyCard
from keycard.constants import DerivationOption


def main():
    if len(sys.argv) != 5:
        print(__doc__)
        sys.exit(2)

    index = int(sys.argv[1])
    key = bytes.fromhex(sys.argv[2])
    path = sys.argv[3]
    digest = bytes.fromhex(sys.argv[4])

    if len(digest) != 32:
        print(f"digest must be 32 bytes, got {len(digest)}")
        sys.exit(2)

    card = KeyCard()
    info = card.select()
    print(f"applet {info.version_major}.{info.version_minor}")

    card.open_secure_channel(index, key)
    print("secure channel open")

    pin = getpass.getpass("PIN: ")
    if not pin.isdigit():
        print("PIN must be digits. Nothing sent.")
        sys.exit(2)

    if not card.verify_pin(pin):
        print("PIN REJECTED")
        print(card.status)
        sys.exit(1)

    print("PIN verified")

    exported = card.export_key(
        derivation_option=DerivationOption.DERIVE,
        public_only=True,
        keypath=path,
        make_current=False,
    )

    print()
    print(f"PATH   {path}")
    print(f"POINT  {exported.public_key.hex()}")

    result = card.sign_with_path(digest, path, make_current=False)

    print(f"SIG    {result}")


if __name__ == "__main__":
    main()
