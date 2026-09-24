#!/usr/bin/env python3
"""
Verify PIN and export a public key.

Prompts for the PIN so it is not stored in shell history.
The only state change is the PIN retry counter, and only if the PIN is wrong.

Usage:
    python3 verify_and_export.py <index> <pairing_key_hex> <path>
"""

import getpass
import sys

from keycard.keycard import KeyCard
from keycard.constants import DerivationOption


def main():
    if len(sys.argv) != 4:
        print(__doc__)
        sys.exit(2)

    index = int(sys.argv[1])
    key = bytes.fromhex(sys.argv[2])
    path = sys.argv[3]

    card = KeyCard()
    info = card.select()
    print(f"applet {info.version_major}.{info.version_minor}")

    card.open_secure_channel(index, key)
    print("secure channel open")

    print(f"pin retries before: {card.status['pin_retry_count']}")

    pin = getpass.getpass("PIN: ")

    if not pin.isdigit():
        print("PIN must be digits only. Nothing sent to the card.")
        sys.exit(2)

    print(f"(sending {len(pin)} digits)")

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
    print(f"path: {path}")
    print(f"exported: {exported}")


if __name__ == "__main__":
    main()
