/*
 * DER -> raw r||s conversion tests.
 *
 * Covers the cases that break naive parsers:
 *   - high bit set, so DER carries a leading 0x00
 *   - short values needing left-padding
 *   - malformed input that must be rejected
 */

#include <stdio.h>
#include <string.h>

#include "ecdsa_der.h"

static int failures = 0;

static void check(const char *name, int condition)
{
    printf("%-44s %s\n", name, condition ? "PASS" : "FAIL");
    if (!condition)
        failures++;
}

/* Both r and s are full 32 bytes with high bit clear. */
static void test_plain(void)
{
    uint8_t der[70];
    uint8_t out[64];
    size_t p = 0;
    int i;

    der[p++] = 0x30;
    der[p++] = 68;
    der[p++] = 0x02;
    der[p++] = 32;
    for (i = 0; i < 32; i++)
        der[p++] = 0x11;
    der[p++] = 0x02;
    der[p++] = 32;
    for (i = 0; i < 32; i++)
        der[p++] = 0x22;

    check("plain 32/32 parses", ecdsa_der_to_raw(der, p, out) == 0);
    check("plain r correct", out[0] == 0x11 && out[31] == 0x11);
    check("plain s correct", out[32] == 0x22 && out[63] == 0x22);
}

/* r has the high bit set, so DER prefixes 0x00 and length is 33. */
static void test_high_bit(void)
{
    uint8_t der[72];
    uint8_t out[64];
    size_t p = 0;
    int i;

    der[p++] = 0x30;
    der[p++] = 69;
    der[p++] = 0x02;
    der[p++] = 33;
    der[p++] = 0x00;
    der[p++] = 0xff;
    for (i = 0; i < 31; i++)
        der[p++] = 0xaa;
    der[p++] = 0x02;
    der[p++] = 32;
    for (i = 0; i < 32; i++)
        der[p++] = 0x33;

    check("high-bit r parses", ecdsa_der_to_raw(der, p, out) == 0);
    check("leading zero stripped", out[0] == 0xff);
    check("high-bit r tail correct", out[31] == 0xaa);
    check("high-bit s correct", out[32] == 0x33);
}

/* s is only 30 bytes and must be left-padded to 32. */
static void test_short(void)
{
    uint8_t der[70];
    uint8_t out[64];
    size_t p = 0;
    int i;

    der[p++] = 0x30;
    der[p++] = 66;
    der[p++] = 0x02;
    der[p++] = 32;
    for (i = 0; i < 32; i++)
        der[p++] = 0x44;
    der[p++] = 0x02;
    der[p++] = 30;
    for (i = 0; i < 30; i++)
        der[p++] = 0x55;

    check("short s parses", ecdsa_der_to_raw(der, p, out) == 0);
    check("short s left-padded", out[32] == 0x00 && out[33] == 0x00);
    check("short s value placed", out[34] == 0x55 && out[63] == 0x55);
}

static void test_rejects(void)
{
    uint8_t out[64];
    uint8_t bad_tag[] = { 0x31, 0x06, 0x02, 0x01, 0x01, 0x02, 0x01, 0x01 };
    uint8_t truncated[] = { 0x30, 0x44, 0x02, 0x20, 0x01 };
    uint8_t trailing[] = {
        0x30, 0x06, 0x02, 0x01, 0x01, 0x02, 0x01, 0x01, 0xff
    };

    check("rejects wrong tag",
          ecdsa_der_to_raw(bad_tag, sizeof(bad_tag), out) != 0);
    check("rejects truncated",
          ecdsa_der_to_raw(truncated, sizeof(truncated), out) != 0);
    check("rejects trailing bytes",
          ecdsa_der_to_raw(trailing, sizeof(trailing), out) != 0);
    check("rejects null", ecdsa_der_to_raw(NULL, 8, out) != 0);
}

int main(void)
{
    test_plain();
    test_high_bit();
    test_short();
    test_rejects();

    printf("\n%s\n", failures ? "FAILURES" : "all passed");
    return failures ? 1 : 0;
}
