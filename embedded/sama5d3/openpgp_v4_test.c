#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "openpgp_v4.h"

int main(void)
{
    static const uint8_t message[] =
        "I control Ethereum address "
        "0x9ce2e20fc392304fd1e50541ec67168913b5f3ff";

    static const uint8_t fingerprint[OPENPGP_V4_FINGERPRINT_LEN] = {
        0x31, 0xce, 0x69, 0xd6, 0x6a,
        0x5e, 0x9d, 0xe0, 0xf9, 0x77,
        0xb5, 0x9c, 0x79, 0xbb, 0x39,
        0x14, 0x97, 0xe8, 0xe6, 0xd4
    };

    static const uint8_t expected_fields[OPENPGP_V4_SIG_FIELDS_LEN] = {
        0x04, 0x01, 0x13, 0x08, 0x00, 0x1d,

        0x16, 0x21, 0x04,
        0x31, 0xce, 0x69, 0xd6, 0x6a,
        0x5e, 0x9d, 0xe0, 0xf9, 0x77,
        0xb5, 0x9c, 0x79, 0xbb, 0x39,
        0x14, 0x97, 0xe8, 0xe6, 0xd4,

        0x05, 0x02, 0x6a, 0xac, 0x6f, 0x88
    };

    static const uint8_t expected_digest[OPENPGP_SHA256_LEN] = {
        0xd5, 0xc2, 0xcf, 0x31, 0xba, 0xa9, 0x91, 0xbb,
        0x51, 0x62, 0x62, 0xad, 0xfe, 0xd4, 0xb3, 0xe0,
        0xbd, 0x53, 0x8a, 0x65, 0x26, 0xe5, 0xdf, 0x02,
        0xb6, 0x5f, 0x26, 0xba, 0x68, 0x15, 0x3d, 0xc3
    };

    uint8_t fields[OPENPGP_V4_SIG_FIELDS_LEN];
    uint8_t digest[OPENPGP_SHA256_LEN];
    size_t fields_len = 0;

    if (openpgp_v4_build_sig_fields(
            fingerprint,
            0x6aac6f88,
            fields,
            sizeof(fields),
            &fields_len) != 0) {
        fprintf(stderr, "signature field construction failed\n");
        return 1;
    }

    printf("fields: ");

    for (size_t i = 0; i < fields_len; i++)
        printf("%02x", fields[i]);

    printf("\n");

    if (fields_len != sizeof(expected_fields) ||
        memcmp(fields, expected_fields, sizeof(expected_fields)) != 0) {
        fprintf(stderr, "SIGNATURE FIELDS MISMATCH\n");
        return 1;
    }

    if (openpgp_v4_digest(
            message,
            sizeof(message) - 1,
            fields,
            fields_len,
            digest) != 0) {
        fprintf(stderr, "digest construction failed\n");
        return 1;
    }

    printf("digest: ");

    for (size_t i = 0; i < sizeof(digest); i++)
        printf("%02x", digest[i]);

    printf("\n");

    if (memcmp(digest, expected_digest, sizeof(expected_digest)) != 0) {
        fprintf(stderr, "OPENPGP DIGEST MISMATCH\n");
        return 1;
    }

    printf("OPENPGP FIELD BUILDER PASS\n");
    return 0;
}
