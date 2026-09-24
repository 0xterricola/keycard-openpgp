/*
 * Build an OpenPGP v4 public-key packet body from a raw SEC1 point.
 *
 * This is the piece missing for a Keycard backend: Keycard's EXPORT KEY
 * returns a bare uncompressed point, whereas every existing function in
 * openpgp_v4.c consumes an already-assembled primary_key_body parsed from
 * a scanned packet bundle.
 *
 * v4 public-key packet body layout (RFC 4880 5.5.2):
 *
 *   version            1 byte   0x04
 *   creation time      4 bytes  big-endian unix seconds
 *   algorithm          1 byte   19 = ECDSA
 *   OID length         1 byte
 *   OID                n bytes
 *   MPI(point)         2 + m    bit length, then the point
 *
 * The creation time is part of the fingerprint preimage. Changing it
 * changes the key's identity, so callers must treat it as fixed.
 */

#include <string.h>

#include "openpgp_pubkey_body.h"

static const uint8_t secp256k1_oid[] = {
    0x2b, 0x81, 0x04, 0x00, 0x0a
};

#define OPENPGP_ALGO_ECDSA 19

static size_t mpi_bit_length(const uint8_t *data, size_t len)
{
    size_t i;
    uint8_t byte;
    size_t bits;

    for (i = 0; i < len; i++) {
        if (data[i] != 0)
            break;
    }

    if (i == len)
        return 0;

    byte = data[i];
    bits = (len - i - 1) * 8;

    while (byte) {
        bits++;
        byte >>= 1;
    }

    return bits;
}

int openpgp_v4_build_public_key_body(
    const uint8_t *point,
    size_t point_len,
    uint32_t creation_time,
    uint8_t *out,
    size_t out_capacity,
    size_t *out_len)
{
    size_t p = 0;
    size_t bits;
    size_t needed;

    if (!point || !out || !out_len)
        return -1;

    if (point_len != 65 || point[0] != 0x04)
        return -1;

    needed = 1 + 4 + 1 + 1 + sizeof(secp256k1_oid) + 2 + point_len;

    if (out_capacity < needed)
        return -1;

    out[p++] = 0x04;

    out[p++] = (uint8_t)(creation_time >> 24);
    out[p++] = (uint8_t)(creation_time >> 16);
    out[p++] = (uint8_t)(creation_time >> 8);
    out[p++] = (uint8_t)creation_time;

    out[p++] = OPENPGP_ALGO_ECDSA;

    out[p++] = (uint8_t)sizeof(secp256k1_oid);
    memcpy(&out[p], secp256k1_oid, sizeof(secp256k1_oid));
    p += sizeof(secp256k1_oid);

    bits = mpi_bit_length(point, point_len);

    out[p++] = (uint8_t)(bits >> 8);
    out[p++] = (uint8_t)bits;

    memcpy(&out[p], point, point_len);
    p += point_len;

    *out_len = p;
    return 0;
}
