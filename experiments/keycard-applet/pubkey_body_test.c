/*
 * Validate openpgp_v4_build_public_key_body against a known-good key.
 *
 * Vector: the NeoPGP secp256k1 signing key
 *   fingerprint  31CE69D66A5E9DE0F977B59C79BB391497E8E6D4
 *   created      2026-09-17 22:32:18 UTC (0x6aac6a72)
 *
 * Expected body bytes taken from `gpg --export` output.
 */

#include <stdio.h>
#include <string.h>

#include "openpgp_pubkey_body.h"
#include "../../embedded/sama5d3/openpgp_v4.h"

static const uint8_t expected_body[] = {
    0x04,
    0x6a, 0xac, 0x6a, 0x72,
    0x13,
    0x05,
    0x2b, 0x81, 0x04, 0x00, 0x0a,
    0x02, 0x03,
    0x04, 0x2a, 0x07, 0x34, 0x94, 0x57, 0xa5, 0x2c,
    0x5a, 0x75, 0x5b, 0xc1, 0xe6, 0x75, 0x00, 0x60,
    0x1f, 0x64, 0xed, 0xf2, 0xad, 0x37, 0x8d, 0x16,
    0x42, 0xcd, 0x4b, 0x32, 0xb2, 0xb5, 0xb6, 0xbb,
    0x16, 0x06, 0xbb, 0xd1, 0x1f, 0x03, 0x29, 0xfd,
    0x3e, 0x9a, 0x5b, 0xa5, 0xc5, 0x5a, 0xc1, 0x18,
    0xda, 0x54, 0x0d, 0xa0, 0x31, 0x74, 0xdd, 0x3a,
    0x96, 0x86, 0xc2, 0xb7, 0xbf, 0x5b, 0xe3, 0xfe,
    0x39
};

static const uint8_t expected_fingerprint[20] = {
    0x31, 0xce, 0x69, 0xd6, 0x6a, 0x5e, 0x9d, 0xe0,
    0xf9, 0x77, 0xb5, 0x9c, 0x79, 0xbb, 0x39, 0x14,
    0x97, 0xe8, 0xe6, 0xd4
};

int main(void)
{
    const uint8_t *point = expected_body + 14;
    const size_t point_len = 65;
    const uint32_t creation_time = 0x6aac6a72;

    uint8_t body[256];
    size_t body_len = 0;
    uint8_t fingerprint[OPENPGP_V4_FINGERPRINT_LEN];
    size_t i;
    int failures = 0;

    if (openpgp_v4_build_public_key_body(
            point, point_len, creation_time,
            body, sizeof(body), &body_len) != 0) {
        printf("FAIL: builder returned error\n");
        return 1;
    }

    printf("built body: %zu bytes (expected %zu)\n",
           body_len, sizeof(expected_body));

    if (body_len != sizeof(expected_body)) {
        printf("FAIL: length mismatch\n");
        failures++;
    } else if (memcmp(body, expected_body, body_len) != 0) {
        printf("FAIL: body bytes differ\n");
        for (i = 0; i < body_len; i++) {
            if (body[i] != expected_body[i])
                printf("  [%zu] got %02x want %02x\n",
                       i, body[i], expected_body[i]);
        }
        failures++;
    } else {
        printf("PASS: body matches gpg --export exactly\n");
    }

    if (openpgp_v4_primary_key_fingerprint(
            body, body_len, fingerprint) != 0) {
        printf("FAIL: fingerprint derivation error\n");
        return 1;
    }

    printf("fingerprint: ");
    for (i = 0; i < sizeof(fingerprint); i++)
        printf("%02x", fingerprint[i]);
    printf("\n");

    if (memcmp(fingerprint, expected_fingerprint,
               sizeof(fingerprint)) != 0) {
        printf("FAIL: fingerprint mismatch\n");
        failures++;
    } else {
        printf("PASS: fingerprint is 31CE69D6...\n");
    }

    return failures ? 1 : 0;
}
