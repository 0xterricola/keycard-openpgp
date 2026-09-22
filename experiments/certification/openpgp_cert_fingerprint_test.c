#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "openpgp_v4.h"

/*
 * Test fixture only.
 *
 * Published Thurin Labs OpenPGP v4 primary key.
 * Reusable OpenPGP code contains no Thurin-specific data.
 */
static const uint8_t target_primary_key_body[] = {
    0x04, 0x6a, 0xa5, 0x86, 0x0c, 0x16, 0x09, 0x2b,
    0x06, 0x01, 0x04, 0x01, 0xda, 0x47, 0x0f, 0x01,
    0x01, 0x07, 0x40, 0xe0, 0xad, 0x14, 0x15, 0x77,
    0x9b, 0xb4, 0xe0, 0x77, 0xff, 0x19, 0x93, 0xa7,
    0x65, 0xda, 0x2d, 0xac, 0x53, 0x95, 0xa2, 0xcb,
    0x4b, 0xef, 0xdc, 0xb4, 0xb1, 0xed, 0x81, 0x07,
    0x67, 0x88, 0xc8
};

static const uint8_t expected_fingerprint[OPENPGP_V4_FINGERPRINT_LEN] = {
    0x08, 0xb9, 0x37, 0x4f, 0xdf,
    0xbe, 0xc6, 0x7e, 0xff, 0xa2,
    0x4e, 0x66, 0x9d, 0x3d, 0x86,
    0xe3, 0x53, 0x61, 0xef, 0x7b
};

int main(void)
{
    uint8_t fingerprint[OPENPGP_V4_FINGERPRINT_LEN];

    if (openpgp_v4_primary_key_fingerprint(
            target_primary_key_body,
            sizeof(target_primary_key_body),
            fingerprint) != 0) {
        fprintf(stderr, "Could not compute target fingerprint\n");
        return 1;
    }

    printf("Target fingerprint: ");

    for (size_t i = 0; i < sizeof(fingerprint); i++)
        printf("%02X", fingerprint[i]);

    printf("\n");

    if (memcmp(fingerprint,
               expected_fingerprint,
               sizeof(fingerprint)) != 0) {
        fprintf(stderr, "Fingerprint vector: FAIL\n");
        return 1;
    }

    printf("Fingerprint vector: PASS\n");
    return 0;
}
