#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "openpgp_v4.h"
#include "neopgp_sign.h"

static int write_file(const char *path,
                      const uint8_t *data,
                      size_t len)
{
    FILE *f = fopen(path, "wb");

    if (!f)
        return -1;

    if (fwrite(data, 1, len, f) != len) {
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}

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

    static const uint8_t key_id[8] = {
        0x79, 0xbb, 0x39, 0x14,
        0x97, 0xe8, 0xe6, 0xd4
    };

    uint8_t fields[OPENPGP_V4_SIG_FIELDS_LEN];
    uint8_t digest[OPENPGP_SHA256_LEN];
    uint8_t raw_signature[OPENPGP_RAW_ECDSA_LEN];
    uint8_t packet[256];

    size_t fields_len = 0;
    size_t raw_signature_len = sizeof(raw_signature);
    size_t packet_len = 0;

    if (openpgp_v4_build_sig_fields(
            fingerprint,
            0x6aac6f88,
            fields,
            sizeof(fields),
            &fields_len) != 0) {
        fprintf(stderr, "Could not build signature fields\n");
        return 1;
    }

    if (openpgp_v4_digest(
            message,
            sizeof(message) - 1,
            fields,
            fields_len,
            digest) != 0) {
        fprintf(stderr, "Could not build OpenPGP digest\n");
        return 1;
    }

    printf("OpenPGP digest: ");

    for (size_t i = 0; i < sizeof(digest); i++)
        printf("%02x", digest[i]);

    printf("\n");
    printf("Enter NeoPGP PIN on keypad and press APPROVE\n");

    if (neopgp_sign_digest(
            digest,
            sizeof(digest),
            raw_signature,
            &raw_signature_len,
            NULL) != 0) {
        fprintf(stderr, "Hardware signing failed\n");
        return 1;
    }

    if (raw_signature_len != OPENPGP_RAW_ECDSA_LEN) {
        fprintf(stderr,
                "Unexpected raw signature length: %zu\n",
                raw_signature_len);
        return 1;
    }

    printf("Raw ECDSA signature: ");

    for (size_t i = 0; i < raw_signature_len; i++)
        printf("%02x", raw_signature[i]);

    printf("\n");

    if (openpgp_v4_build_signature_packet(
            fields,
            fields_len,
            digest,
            raw_signature,
            key_id,
            packet,
            sizeof(packet),
            &packet_len) != 0) {
        fprintf(stderr, "Could not build signature packet\n");
        return 1;
    }

    if (write_file("/tmp/openpgp-message.txt",
                   message,
                   sizeof(message) - 1) != 0) {
        fprintf(stderr, "Could not write message\n");
        return 1;
    }

    if (write_file("/tmp/openpgp-detached.sig",
                   packet,
                   packet_len) != 0) {
        fprintf(stderr, "Could not write signature packet\n");
        return 1;
    }

    printf("OpenPGP signature packet: %zu bytes\n", packet_len);
    printf("Wrote /tmp/openpgp-message.txt\n");
    printf("Wrote /tmp/openpgp-detached.sig\n");

    return 0;
}
