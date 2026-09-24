#!/usr/bin/env python3
"""
Open a secure channel and read card status. Read-only.

Usage:
    python3 card_status.py <pairing_index> <pairing_key_hex>
"""

import sys
from keycard.keycard import KeyCard


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(2)

    index = int(sys.argv[1])
    key = bytes.fromhex(sys.argv[2])

    card = KeyCard()
    info = card.select()
    print(f"applet {info.version_major}.{info.version_minor}")

    card.open_secure_channel(index, key)
    print("secure channel open")

    print(card.status)


if __name__ == "__main__":
    main()
