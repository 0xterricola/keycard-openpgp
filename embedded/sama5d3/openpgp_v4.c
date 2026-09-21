#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <openssl/evp.h>

#include "openpgp_v4.h"

int openpgp_v4_build_sig_fields(
    const uint8_t fingerprint[OPENPGP_V4_FINGERPRINT_LEN],
    uint32_t creation_time,
    uint8_t *out,
    size_t out_capacity,
    size_t *out_len)
{
    size_t p = 0;

    if (!fingerprint || !out || !out_len)
        return -1;

    if (out_capacity < OPENPGP_V4_SIG_FIELDS_LEN)
        return -1;

    /* Version 4 signature. */
    out[p++] = 0x04;

    /* Signature type 0x01: canonical text document. */
    out[p++] = 0x01;

    /* Public-key algorithm 19: ECDSA. */
    out[p++] = 0x13;

    /* Hash algorithm 8: SHA-256. */
    out[p++] = 0x08;

    /*
     * Hashed subpacket data is 29 bytes:
     *
     *   issuer fingerprint:
     *     16 21 04 <20-byte fingerprint>
     *
     *   signature creation time:
     *     05 02 <4-byte timestamp>
     */
    out[p++] = 0x00;
    out[p++] = 0x1d;

    /* Issuer Fingerprint subpacket. */
    out[p++] = 0x16;
    out[p++] = 0x21;
    out[p++] = 0x04;

    memcpy(&out[p], fingerprint, OPENPGP_V4_FINGERPRINT_LEN);
    p += OPENPGP_V4_FINGERPRINT_LEN;

    /* Signature Creation Time subpacket. */
    out[p++] = 0x05;
    out[p++] = 0x02;

    out[p++] = (uint8_t)(creation_time >> 24);
    out[p++] = (uint8_t)(creation_time >> 16);
    out[p++] = (uint8_t)(creation_time >> 8);
    out[p++] = (uint8_t)creation_time;

    if (p != OPENPGP_V4_SIG_FIELDS_LEN)
        return -1;

    *out_len = p;
    return 0;
}

int openpgp_v4_digest(const uint8_t *signed_data,
                      size_t signed_data_len,
                      const uint8_t *sig_fields,
                      size_t sig_fields_len,
                      uint8_t digest[OPENPGP_SHA256_LEN])
{
    EVP_MD_CTX *ctx = NULL;
    unsigned int digest_len = 0;
    uint8_t trailer[6];
    uint32_t n;
    int result = -1;

    if (!signed_data || !sig_fields || !digest)
        return -1;

    if (sig_fields_len == 0 ||
        sig_fields[0] != 0x04 ||
        sig_fields_len > UINT32_MAX)
        return -1;

    n = (uint32_t)sig_fields_len;

    trailer[0] = 0x04;
    trailer[1] = 0xff;
    trailer[2] = (uint8_t)(n >> 24);
    trailer[3] = (uint8_t)(n >> 16);
    trailer[4] = (uint8_t)(n >> 8);
    trailer[5] = (uint8_t)n;

    ctx = EVP_MD_CTX_new();
    if (!ctx)
        return -1;

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1)
        goto out;

    if (EVP_DigestUpdate(ctx, signed_data, signed_data_len) != 1)
        goto out;

    if (EVP_DigestUpdate(ctx, sig_fields, sig_fields_len) != 1)
        goto out;

    if (EVP_DigestUpdate(ctx, trailer, sizeof(trailer)) != 1)
        goto out;

    if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1)
        goto out;

    if (digest_len != OPENPGP_SHA256_LEN)
        goto out;

    result = 0;

out:
    EVP_MD_CTX_free(ctx);
    return result;
}

static int encode_mpi(const uint8_t *value,
                      size_t value_len,
                      uint8_t *out,
                      size_t out_capacity,
                      size_t *out_len)
{
    size_t start = 0;
    unsigned leading = 0;
    unsigned bit_len;
    uint8_t first;

    while (start < value_len && value[start] == 0)
        start++;

    if (start == value_len)
        return -1;

    first = value[start];

    while ((first & 0x80) == 0) {
        leading++;
        first <<= 1;
    }

    bit_len = (unsigned)((value_len - start) * 8) - leading;

    if (out_capacity < 2 + value_len - start)
        return -1;

    out[0] = (uint8_t)(bit_len >> 8);
    out[1] = (uint8_t)bit_len;

    memcpy(&out[2], &value[start], value_len - start);

    *out_len = 2 + value_len - start;
    return 0;
}

int openpgp_v4_build_signature_packet(
    const uint8_t *sig_fields,
    size_t sig_fields_len,
    const uint8_t digest[OPENPGP_SHA256_LEN],
    const uint8_t raw_signature[OPENPGP_RAW_ECDSA_LEN],
    const uint8_t issuer_key_id[8],
    uint8_t *out,
    size_t out_capacity,
    size_t *out_len)
{
    uint8_t body[160];
    size_t p = 0;
    size_t mpi_len;

    if (!sig_fields || !digest || !raw_signature ||
        !issuer_key_id || !out || !out_len)
        return -1;

    if (sig_fields_len != OPENPGP_V4_SIG_FIELDS_LEN)
        return -1;

    memcpy(&body[p], sig_fields, sig_fields_len);
    p += sig_fields_len;

    /*
     * Unhashed subpacket area:
     *
     *   length = 10 bytes
     *   subpacket length = 9
     *   type 16 = Issuer Key ID
     *   8-byte key ID
     */
    body[p++] = 0x00;
    body[p++] = 0x0a;

    body[p++] = 0x09;
    body[p++] = 0x10;

    memcpy(&body[p], issuer_key_id, 8);
    p += 8;

    /* Leftmost 16 bits of the signed hash. */
    body[p++] = digest[0];
    body[p++] = digest[1];

    /* ECDSA r MPI. */
    if (encode_mpi(&raw_signature[0],
                   32,
                   &body[p],
                   sizeof(body) - p,
                   &mpi_len) != 0)
        return -1;

    p += mpi_len;

    /* ECDSA s MPI. */
    if (encode_mpi(&raw_signature[32],
                   32,
                   &body[p],
                   sizeof(body) - p,
                   &mpi_len) != 0)
        return -1;

    p += mpi_len;

    /*
     * New-format packet header, tag 2 = Signature Packet.
     * Our body is comfortably below 192 bytes.
     */
    if (p >= 192 || out_capacity < p + 2)
        return -1;

    out[0] = 0xc2;
    out[1] = (uint8_t)p;

    memcpy(&out[2], body, p);

    *out_len = p + 2;
    return 0;
}
