#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "openpgp_v4.h"

/*
 * Demo/test fixture only.
 *
 * Target:
 *   Thurin Labs <hello@thurin.id>
 *
 * Primary fingerprint:
 *   08B9374FDFBEC67EFFA24E669D3D86E35361EF7B
 *
 * The reusable OpenPGP implementation contains no Thurin-specific data.
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

static const uint8_t target_uid[] =
    "Thurin Labs <hello@thurin.id>";

static const uint8_t signer_fingerprint[OPENPGP_V4_FINGERPRINT_LEN] = {
    0x31, 0xce, 0x69, 0xd6, 0x6a,
    0x5e, 0x9d, 0xe0, 0xf9, 0x77,
    0xb5, 0x9c, 0x79, 0xbb, 0x39,
    0x14, 0x97, 0xe8, 0xe6, 0xd4
};

/*
 * Fixed test timestamp, matching the existing OpenPGP vector tests.
 * The later trusted-device flow will use device-controlled operation time.
 */
static const uint32_t creation_time = 0x6aac6f88;

/*
 * Independently calculated reference digest for:
 *
 *   v4 primary-key certification
 *   target UID above
 *   certification type 0x10
 *   ECDSA
 *   SHA-256
 *   issuer fingerprint above
 *   creation_time above
 */
static const uint8_t expected_digest[OPENPGP_SHA256_LEN] = {
    0x94, 0xe9, 0x37, 0x19, 0xb6, 0x40, 0xa2, 0x28,
    0xef, 0x11, 0xe4, 0xf4, 0x50, 0xc7, 0x60, 0x80,
    0x78, 0xd8, 0x35, 0xd7, 0x85, 0x96, 0x64, 0x5f,
    0xf4, 0x2d, 0x97, 0xa6, 0x1b, 0x56, 0x40, 0x0a
};

int main(void)
{
    uint8_t certification_data[128];
    uint8_t sig_fields[OPENPGP_V4_SIG_FIELDS_LEN];
    uint8_t digest[OPENPGP_SHA256_LEN];

    size_t certification_data_len = 0;
    size_t sig_fields_len = 0;

    if (sizeof(target_primary_key_body) != 51) {
        fprintf(stderr, "Unexpected primary-key body length\n");
        return 1;
    }

    if (sizeof(target_uid) - 1 != 29) {
        fprintf(stderr, "Unexpected UID length\n");
        return 1;
    }

    if (openpgp_v4_build_certification_data(
            target_primary_key_body,
            sizeof(target_primary_key_body),
            target_uid,
            sizeof(target_uid) - 1,
            certification_data,
            sizeof(certification_data),
            &certification_data_len) != 0) {
        fprintf(stderr, "Could not construct certification data\n");
        return 1;
    }

    if (certification_data_len != 88) {
        fprintf(stderr,
                "Unexpected certification-data length: %zu\n",
                certification_data_len);
        return 1;
    }

    /*
     * 0x10 = generic certification.
     */
    if (openpgp_v4_build_sig_fields_for_type(
            0x10,
            signer_fingerprint,
            creation_time,
            sig_fields,
            sizeof(sig_fields),
            &sig_fields_len) != 0) {
        fprintf(stderr, "Could not construct signature fields\n");
        return 1;
    }

    if (openpgp_v4_digest(
            certification_data,
            certification_data_len,
            sig_fields,
            sig_fields_len,
            digest) != 0) {
        fprintf(stderr, "Could not construct certification digest\n");
        return 1;
    }

    printf("Certification data: %zu bytes\n", certification_data_len);
    printf("Certification digest: ");

    for (size_t i = 0; i < sizeof(digest); i++)
        printf("%02x", digest[i]);

    printf("\n");

    if (memcmp(digest, expected_digest, sizeof(digest)) != 0) {
        fprintf(stderr, "Digest vector: FAIL\n");
        return 1;
    }

    printf("Digest vector: PASS\n");
    return 0;
}
