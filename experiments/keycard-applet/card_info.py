#!/usr/bin/env python3
"""
Read-only Keycard inventory.

Sends SELECT only. No pairing, no secure channel, no PIN, no writes.
Safe to run against a card holding real keys.
"""

from keycard.keycard import KeyCard


def show(label, value):
    print(f"{label:<28} {value}")


def main():
    card = KeyCard()
    info = card.select()

    print("SELECT response")
    print("-" * 60)

    for attr in sorted(a for a in dir(info) if not a.startswith("_")):
        value = getattr(info, attr)
        if callable(value):
            continue
        if isinstance(value, (bytes, bytearray)):
            value = value.hex()
        show(attr, value)

    print("-" * 60)
    print("raw:", info)


if __name__ == "__main__":
    main()
