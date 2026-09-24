#!/usr/bin/env python3
"""
Single pairing attempt against a Keycard.

ONE attempt, no retries. Failed attempts are counted by the card.

Usage:
    python3 pair_once.py "PairingPassword"
"""

import sys

from keycard.keycard import KeyCard


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(2)

    card = KeyCard()
    info = card.select()
    print(f"applet {info.version_major}.{info.version_minor}  "
          f"initialized={info.is_initialized}")

    try:
        index, pairing_key = card.pair(sys.argv[1])
    except Exception as exc:
        print(f"PAIRING FAILED: {exc}")
        sys.exit(1)

    print()
    print(f"PAIRED  index={index}")
    print(f"PAIRING KEY (SAVE THIS): {pairing_key.hex()}")
    print()
    print("Release this slot with:")
    print(f"    python3 unpair.py {index} {pairing_key.hex()}")


if __name__ == "__main__":
    main()
