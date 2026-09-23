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
    return openpgp_v4_build_sig_fields_for_type(
        0x01,
        fingerprint,
        creation_time,
        out,
        out_capacity,
        out_len);
}

int openpgp_v4_build_sig_fields_for_type(
    uint8_t signature_type,
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

    /* Signature type selected by the trusted protocol operation. */
    out[p++] = signature_type;

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

int openpgp_v4_build_certification_data(
    const uint8_t *primary_key_body,
    size_t primary_key_body_len,
    const uint8_t *user_id,
    size_t user_id_len,
    uint8_t *out,
    size_t out_capacity,
    size_t *out_len)
{
    size_t p = 0;
    uint32_t uid_len32;

    if (!primary_key_body || !user_id || !out || !out_len)
        return -1;

    if (primary_key_body_len > UINT16_MAX ||
        user_id_len > UINT32_MAX)
        return -1;

    /*
     * OpenPGP v4 certification hashes the primary-key packet body as:
     *
     *   0x99 || uint16(body length) || body
     */
    if (out_capacity < 3)
        return -1;

    out[p++] = 0x99;
    out[p++] = (uint8_t)(primary_key_body_len >> 8);
    out[p++] = (uint8_t)primary_key_body_len;

    if (primary_key_body_len > out_capacity - p)
        return -1;

    memcpy(&out[p], primary_key_body, primary_key_body_len);
    p += primary_key_body_len;

    /*
     * A v4 User-ID certification adds:
     *
     *   0xB4 || uint32(uid length) || uid
     */
    if (out_capacity - p < 5)
        return -1;

    uid_len32 = (uint32_t)user_id_len;

    out[p++] = 0xb4;
    out[p++] = (uint8_t)(uid_len32 >> 24);
    out[p++] = (uint8_t)(uid_len32 >> 16);
    out[p++] = (uint8_t)(uid_len32 >> 8);
    out[p++] = (uint8_t)uid_len32;

    if (user_id_len > out_capacity - p)
        return -1;

    memcpy(&out[p], user_id, user_id_len);
    p += user_id_len;

    *out_len = p;
    return 0;
}

int openpgp_v4_primary_key_fingerprint(
    const uint8_t *primary_key_body,
    size_t primary_key_body_len,
    uint8_t fingerprint[OPENPGP_V4_FINGERPRINT_LEN])
{
    EVP_MD_CTX *ctx = NULL;
    unsigned int digest_len = 0;
    uint8_t prefix[3];
    int result = -1;

    if (!primary_key_body || !fingerprint)
        return -1;

    if (primary_key_body_len == 0 ||
        primary_key_body_len > UINT16_MAX ||
        primary_key_body[0] != 0x04)
        return -1;

    /*
     * OpenPGP v4 primary-key fingerprint:
     *
     *   SHA1(0x99 || uint16(key-body length) || key-body)
     */
    prefix[0] = 0x99;
    prefix[1] = (uint8_t)(primary_key_body_len >> 8);
    prefix[2] = (uint8_t)primary_key_body_len;

    ctx = EVP_MD_CTX_new();
    if (!ctx)
        return -1;

    if (EVP_DigestInit_ex(ctx, EVP_sha1(), NULL) != 1)
        goto out;

    if (EVP_DigestUpdate(ctx, prefix, sizeof(prefix)) != 1)
        goto out;

    if (EVP_DigestUpdate(ctx,
                         primary_key_body,
                         primary_key_body_len) != 1)
        goto out;

    if (EVP_DigestFinal_ex(ctx, fingerprint, &digest_len) != 1)
        goto out;

    if (digest_len != OPENPGP_V4_FINGERPRINT_LEN)
        goto out;

    result = 0;

out:
    EVP_MD_CTX_free(ctx);
    return result;
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

static uint16_t openpgp_read_be16(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | p[1];
}

static const EVP_MD *openpgp_hash_algorithm(uint8_t algorithm)
{
    switch (algorithm) {
    case 8:  return EVP_sha256();
    case 9:  return EVP_sha384();
    case 10: return EVP_sha512();
    default: return NULL;
    }
}

static int openpgp_read_eddsa_mpi(const uint8_t *data,
                                  size_t data_len,
                                  size_t *offset,
                                  uint8_t out[32])
{
    uint16_t bits;
    size_t bytes;

    if (!data || !offset || !out)
        return -1;

    if (*offset > data_len || data_len - *offset < 2)
        return -1;

    bits = openpgp_read_be16(data + *offset);
    *offset += 2;

    bytes = ((size_t)bits + 7) / 8;

    if (bytes == 0 || bytes > 32)
        return -1;

    if (*offset > data_len || data_len - *offset < bytes)
        return -1;

    /*
     * Ed25519Legacy MPI contents are native little-endian octet
     * strings. If shorter than 32 bytes, omitted high-order zero
     * octets belong at the end of the native representation.
     */
    memset(out, 0, 32);
    memcpy(out, data + *offset, bytes);

    *offset += bytes;
    return 0;
}

int openpgp_v4_verify_uid_self_cert(
    const uint8_t *primary_key_body,
    size_t primary_key_body_len,
    const uint8_t *user_id,
    size_t user_id_len,
    const uint8_t *signature_body,
    size_t signature_body_len)
{
    static const uint8_t ed25519_oid[] = {
        0x2b, 0x06, 0x01, 0x04, 0x01,
        0xda, 0x47, 0x0f, 0x01
    };

    uint8_t public_key[32];
    uint8_t native_signature[64];
    uint8_t digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;

    uint8_t key_prefix[3];
    uint8_t uid_prefix[5];
    uint8_t trailer[6];

    size_t key_pos;
    size_t point_bytes;
    uint16_t point_bits;

    size_t hashed_subpacket_len;
    size_t signature_hashed_len;
    size_t pos;
    size_t unhashed_len;

    const EVP_MD *md = NULL;
    EVP_MD_CTX *hash_ctx = NULL;
    EVP_MD_CTX *verify_ctx = NULL;
    EVP_PKEY *pkey = NULL;

    int result = -1;

    if (!primary_key_body || !user_id || !signature_body)
        return -1;

    if (primary_key_body_len > UINT16_MAX ||
        user_id_len > UINT32_MAX)
        return -1;

    /*
     * This verifier intentionally supports the exact interoperable
     * v4 Ed25519Legacy certificate form used by the target.
     */
    if (primary_key_body_len < 18 ||
        primary_key_body[0] != 0x04 ||
        primary_key_body[5] != 22)
        return -1;

    /*
     * Ed25519Legacy public key:
     *   version + timestamp + algorithm
     *   OID length + Ed25519 OID
     *   MPI(0x40 || 32-byte native public key)
     */
    key_pos = 6;

    if (primary_key_body[key_pos++] != sizeof(ed25519_oid))
        return -1;

    if (primary_key_body_len - key_pos < sizeof(ed25519_oid))
        return -1;

    if (memcmp(primary_key_body + key_pos,
               ed25519_oid,
               sizeof(ed25519_oid)) != 0)
        return -1;

    key_pos += sizeof(ed25519_oid);

    if (primary_key_body_len - key_pos < 2)
        return -1;

    point_bits = openpgp_read_be16(primary_key_body + key_pos);
    key_pos += 2;

    point_bytes = ((size_t)point_bits + 7) / 8;

    if (point_bits != 263 ||
        point_bytes != 33 ||
        primary_key_body_len - key_pos != 33 ||
        primary_key_body[key_pos] != 0x40)
        return -1;

    memcpy(public_key,
           primary_key_body + key_pos + 1,
           sizeof(public_key));

    /*
     * Version-4 certification signature.
     */
    if (signature_body_len < 10 ||
        signature_body[0] != 0x04 ||
        signature_body[1] < 0x10 ||
        signature_body[1] > 0x13 ||
        signature_body[2] != 22)
        return -1;

    md = openpgp_hash_algorithm(signature_body[3]);
    if (!md)
        return -1;

    hashed_subpacket_len =
        openpgp_read_be16(signature_body + 4);

    signature_hashed_len = 6 + hashed_subpacket_len;

    if (signature_hashed_len > signature_body_len)
        return -1;

    /*
     * OpenPGP v4 UID certification hash:
     *
     *   0x99 || uint16(key body length) || key body
     *   0xB4 || uint32(uid length)      || uid
     *   signature fields through hashed subpackets
     *   0x04 || 0xFF || uint32(signature hashed length)
     */
    key_prefix[0] = 0x99;
    key_prefix[1] = (uint8_t)(primary_key_body_len >> 8);
    key_prefix[2] = (uint8_t)primary_key_body_len;

    uid_prefix[0] = 0xB4;
    uid_prefix[1] = (uint8_t)(user_id_len >> 24);
    uid_prefix[2] = (uint8_t)(user_id_len >> 16);
    uid_prefix[3] = (uint8_t)(user_id_len >> 8);
    uid_prefix[4] = (uint8_t)user_id_len;

    trailer[0] = 0x04;
    trailer[1] = 0xFF;
    trailer[2] = (uint8_t)(signature_hashed_len >> 24);
    trailer[3] = (uint8_t)(signature_hashed_len >> 16);
    trailer[4] = (uint8_t)(signature_hashed_len >> 8);
    trailer[5] = (uint8_t)signature_hashed_len;

    hash_ctx = EVP_MD_CTX_new();
    if (!hash_ctx)
        goto out;

    if (EVP_DigestInit_ex(hash_ctx, md, NULL) != 1 ||
        EVP_DigestUpdate(hash_ctx,
                         key_prefix,
                         sizeof(key_prefix)) != 1 ||
        EVP_DigestUpdate(hash_ctx,
                         primary_key_body,
                         primary_key_body_len) != 1 ||
        EVP_DigestUpdate(hash_ctx,
                         uid_prefix,
                         sizeof(uid_prefix)) != 1 ||
        EVP_DigestUpdate(hash_ctx,
                         user_id,
                         user_id_len) != 1 ||
        EVP_DigestUpdate(hash_ctx,
                         signature_body,
                         signature_hashed_len) != 1 ||
        EVP_DigestUpdate(hash_ctx,
                         trailer,
                         sizeof(trailer)) != 1 ||
        EVP_DigestFinal_ex(hash_ctx,
                           digest,
                           &digest_len) != 1)
        goto out;

    /*
     * Skip unhashed subpackets and verify OpenPGP's two-byte
     * signed-hash prefix before doing the public-key operation.
     */
    pos = signature_hashed_len;

    if (signature_body_len - pos < 2)
        goto out;

    unhashed_len = openpgp_read_be16(signature_body + pos);
    pos += 2;

    if (unhashed_len > signature_body_len - pos)
        goto out;

    pos += unhashed_len;

    if (signature_body_len - pos < 2)
        goto out;

    if (signature_body[pos] != digest[0] ||
        signature_body[pos + 1] != digest[1])
        goto out;

    pos += 2;

    if (openpgp_read_eddsa_mpi(
            signature_body,
            signature_body_len,
            &pos,
            native_signature) != 0)
        goto out;

    if (openpgp_read_eddsa_mpi(
            signature_body,
            signature_body_len,
            &pos,
            native_signature + 32) != 0)
        goto out;

    if (pos != signature_body_len)
        goto out;

    /*
     * OpenPGP Ed25519Legacy signs the OpenPGP digest as the EdDSA
     * message. OpenSSL's Ed25519 interface therefore verifies the
     * reconstructed 64-byte native signature over that digest.
     */
    pkey = EVP_PKEY_new_raw_public_key(
        EVP_PKEY_ED25519,
        NULL,
        public_key,
        sizeof(public_key));

    if (!pkey)
        goto out;

    verify_ctx = EVP_MD_CTX_new();
    if (!verify_ctx)
        goto out;

    if (EVP_DigestVerifyInit(
            verify_ctx,
            NULL,
            NULL,
            NULL,
            pkey) != 1)
        goto out;

    if (EVP_DigestVerify(
            verify_ctx,
            native_signature,
            sizeof(native_signature),
            digest,
            digest_len) != 1)
        goto out;

    result = 0;

out:
    EVP_MD_CTX_free(hash_ctx);
    EVP_MD_CTX_free(verify_ctx);
    EVP_PKEY_free(pkey);

    return result;
}
