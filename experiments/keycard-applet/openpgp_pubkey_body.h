#ifndef OPENPGP_PUBKEY_BODY_H
#define OPENPGP_PUBKEY_BODY_H

#include <stddef.h>
#include <stdint.h>

/*
 * Assemble a v4 primary public-key packet body for a secp256k1 ECDSA key
 * from a 65-byte uncompressed SEC1 point.
 *
 * The result is suitable as primary_key_body for the existing
 * openpgp_v4_* functions, including fingerprint derivation.
 */
int openpgp_v4_build_public_key_body(
    const uint8_t *point,
    size_t point_len,
    uint32_t creation_time,
    uint8_t *out,
    size_t out_capacity,
    size_t *out_len);

#endif
