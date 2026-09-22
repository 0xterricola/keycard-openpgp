#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "openpgp_target.h"
#include "openpgp_v4.h"

static const uint8_t expected_fingerprint[OPENPGP_V4_FINGERPRINT_LEN] = {
    0x08, 0xb9, 0x37, 0x4f, 0xdf,
    0xbe, 0xc6, 0x7e, 0xff, 0xa2,
    0x4e, 0x66, 0x9d, 0x3d, 0x86,
    0xe3, 0x53, 0x61, 0xef, 0x7b
};

static int read_file(const char *path, uint8_t **data, size_t *len)
{
    FILE *f;
    long size;
    uint8_t *buf;

    f = fopen(path, "rb");
    if (!f)
        return -1;

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -1;
    }

    size = ftell(f);
    if (size <= 0) {
        fclose(f);
        return -1;
    }

    rewind(f);

    buf = malloc((size_t)size);
    if (!buf) {
        fclose(f);
        return -1;
    }

    if (fread(buf, 1, (size_t)size, f) != (size_t)size) {
        free(buf);
        fclose(f);
        return -1;
    }

    fclose(f);

    *data = buf;
    *len = (size_t)size;
    return 0;
}

int main(int argc, char **argv)
{
    uint8_t *bundle = NULL;
    size_t bundle_len = 0;

    openpgp_cert_target_t target;
    uint8_t fingerprint[OPENPGP_V4_FINGERPRINT_LEN];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <cert-target.pgp>\n", argv[0]);
        return 1;
    }

    if (read_file(argv[1], &bundle, &bundle_len) != 0) {
        fprintf(stderr, "Could not read packet bundle\n");
        return 1;
    }

    if (openpgp_parse_cert_target(
            bundle,
            bundle_len,
            &target) != 0) {
        fprintf(stderr, "Target parser: FAIL\n");
        free(bundle);
        return 1;
    }

    if (openpgp_v4_primary_key_fingerprint(
            target.primary_key_body,
            target.primary_key_body_len,
            fingerprint) != 0) {
        fprintf(stderr, "Fingerprint derivation failed\n");
        free(bundle);
        return 1;
    }

    printf("Bundle: %zu bytes\n", bundle_len);
    printf("Primary-key body: %zu bytes\n",
           target.primary_key_body_len);

    printf("UID packet body: %zu bytes\n",
           target.user_id_len);

    printf("UID extracted from OpenPGP packet: ");
    fwrite(target.user_id, 1, target.user_id_len, stdout);
    printf("\n");

    printf("Self-cert packet: %zu bytes\n",
           target.self_cert_packet_len);

    printf("Self-cert type: 0x%02X\n",
           target.self_cert_type);

    printf("Derived fingerprint: ");

    for (size_t i = 0; i < sizeof(fingerprint); i++)
        printf("%02X", fingerprint[i]);

    printf("\n");

    if (memcmp(fingerprint,
               expected_fingerprint,
               sizeof(fingerprint)) != 0) {
        fprintf(stderr, "Fingerprint mismatch\n");
        free(bundle);
        return 1;
    }

    if (target.primary_key_body_len != 51 ||
        target.user_id_len != 29 ||
        target.self_cert_packet_len != 150 ||
        target.self_cert_type != 0x13) {
        fprintf(stderr, "Unexpected fixture packet structure\n");
        free(bundle);
        return 1;
    }

    printf("Target parser: PASS\n");

    if (openpgp_v4_verify_uid_self_cert(
            target.primary_key_body,
            target.primary_key_body_len,
            target.user_id,
            target.user_id_len,
            target.self_cert_body,
            target.self_cert_body_len) != 0) {
        fprintf(stderr, "Self-cert verification: FAIL\n");
        free(bundle);
        return 1;
    }

    printf("Self-cert verification: PASS\n");

    {
        uint8_t tampered_uid[256];

        if (target.user_id_len > sizeof(tampered_uid)) {
            free(bundle);
            return 1;
        }

        memcpy(tampered_uid,
               target.user_id,
               target.user_id_len);

        tampered_uid[0] ^= 0x01;

        if (openpgp_v4_verify_uid_self_cert(
                target.primary_key_body,
                target.primary_key_body_len,
                tampered_uid,
                target.user_id_len,
                target.self_cert_body,
                target.self_cert_body_len) == 0) {
            fprintf(stderr,
                    "Tampered UID was incorrectly accepted\n");
            free(bundle);
            return 1;
        }
    }

    printf("Tampered UID rejection: PASS\n");

    free(bundle);
    return 0;
}
