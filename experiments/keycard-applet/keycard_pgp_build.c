/*
 * Assemble an OpenPGP identity around a Keycard-held secp256k1 key.
 *
 * Phase 1 (digest):  point + uid            -> certification digest
 * Phase 2 (assemble): point + uid + r||s    -> armored-ready key bytes
 *
 * The creation time is fixed by CREATION_TIME below. It is part of the
 * fingerprint preimage, so changing it changes the key's identity.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openpgp_pubkey_body.h"
#include "openpgp_v4.h"

#define CREATION_TIME 0x69000000u

static int hex_to_bytes(const char *hex, uint8_t *out, size_t out_capacity,
                        size_t *out_len)
{
    size_t len = strlen(hex);
    size_t i;

    if (len % 2 || len / 2 > out_capacity)
        return -1;

    for (i = 0; i < len; i += 2) {
        unsigned int byte;
        if (sscanf(hex + i, "%2x", &byte) != 1)
            return -1;
        out[i / 2] = (uint8_t)byte;
    }

    *out_len = len / 2;
    return 0;
}

static void print_hex(const uint8_t *data, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++)
        printf("%02x", data[i]);
    printf("\n");
}

/* Old-format packet header, definite length. */
static size_t write_packet(uint8_t *out, uint8_t tag,
                           const uint8_t *body, size_t body_len)
{
    size_t p = 0;

    if (body_len < 256) {
        out[p++] = 0x80 | (tag << 2) | 0x00;
        out[p++] = (uint8_t)body_len;
    } else {
        out[p++] = 0x80 | (tag << 2) | 0x01;
        out[p++] = (uint8_t)(body_len >> 8);
        out[p++] = (uint8_t)body_len;
    }

    memcpy(out + p, body, body_len);
    return p + body_len;
}

int main(int argc, char **argv)
{
    uint8_t point[128];
    size_t point_len = 0;
    uint8_t body[256];
    size_t body_len = 0;
    uint8_t fingerprint[OPENPGP_V4_FINGERPRINT_LEN];
    uint8_t cert_data[2048];
    size_t cert_data_len = 0;
    uint8_t sig_fields[OPENPGP_V4_SIG_FIELDS_LEN];
    size_t sig_fields_len = 0;
    uint8_t digest[OPENPGP_SHA256_LEN];
    const char *uid;
    size_t i;

    if (argc < 4) {
        printf("usage:\n");
        printf("  %s digest   <point_hex> <uid>\n", argv[0]);
        printf("  %s assemble <point_hex> <uid> <r_hex> <s_hex>\n", argv[0]);
        return 2;
    }

    uid = argv[3];

    if (hex_to_bytes(argv[2], point, sizeof(point), &point_len) != 0) {
        printf("bad point hex\n");
        return 1;
    }

    if (openpgp_v4_build_public_key_body(
            point, point_len, CREATION_TIME,
            body, sizeof(body), &body_len) != 0) {
        printf("could not build public key body\n");
        return 1;
    }

    if (openpgp_v4_primary_key_fingerprint(
            body, body_len, fingerprint) != 0) {
        printf("could not derive fingerprint\n");
        return 1;
    }

    if (openpgp_v4_build_certification_data(
            body, body_len,
            (const uint8_t *)uid, strlen(uid),
            cert_data, sizeof(cert_data), &cert_data_len) != 0) {
        printf("could not build certification data\n");
        return 1;
    }

    if (openpgp_v4_build_sig_fields_for_type(
            0x13, fingerprint, CREATION_TIME,
            sig_fields, sizeof(sig_fields), &sig_fields_len) != 0) {
        printf("could not build sig fields\n");
        return 1;
    }

    if (openpgp_v4_digest(cert_data, cert_data_len,
                          sig_fields, sig_fields_len, digest) != 0) {
        printf("could not derive digest\n");
        return 1;
    }

    if (strcmp(argv[1], "digest") == 0) {
        printf("fingerprint: ");
        print_hex(fingerprint, sizeof(fingerprint));
        printf("uid:         %s\n", uid);
        printf("creation:    0x%08x\n", CREATION_TIME);
        printf("\nDIGEST TO SIGN:\n");
        print_hex(digest, sizeof(digest));
        return 0;
    }

    if (strcmp(argv[1], "assemble") == 0) {
        uint8_t raw_sig[OPENPGP_RAW_ECDSA_LEN];
        size_t r_len = 0, s_len = 0;
        uint8_t sig_packet[512];
        size_t sig_packet_len = 0;
        uint8_t out[2048];
        size_t p = 0;

        if (argc != 6) {
            printf("assemble needs r_hex and s_hex\n");
            return 2;
        }

        if (hex_to_bytes(argv[4], raw_sig, 32, &r_len) != 0 || r_len != 32) {
            printf("bad r\n");
            return 1;
        }

        if (hex_to_bytes(argv[5], raw_sig + 32, 32, &s_len) != 0 || s_len != 32) {
            printf("bad s\n");
            return 1;
        }

        if (openpgp_v4_build_signature_packet(
                sig_fields, sig_fields_len,
                digest, raw_sig,
                fingerprint + 12,
                sig_packet, sizeof(sig_packet),&sig_packet_len) != 0) {
            printf("could not build signature packet\n");
            return 1;
        }

        p += write_packet(out + p, 6, body, body_len);
        p += write_packet(out + p, 13, (const uint8_t *)uid, strlen(uid));
        memcpy(out + p, sig_packet, sig_packet_len);
        p += sig_packet_len;

        for (i = 0; i < p; i++)
            printf("%02x", out[i]);
        printf("\n");

        return 0;
    }

    printf("unknown mode\n");
    return 2;
}
