/*
 * Host shims for testing Shell's openpgp_v4.c without firmware.
 *
 * Provides the crypto/sha2.h, crypto/ecdsa.h and crypto/secp256k1.h
 * surface that the ported code uses, backed by OpenSSL.
 */
#ifndef SHIMS_H
#define SHIMS_H

#include <stddef.h>
#include <stdint.h>

#define SHA1_DIGEST_LENGTH   20
#define SHA256_DIGEST_LENGTH 32

typedef struct { void *ctx; } SHA1_CTX;
typedef struct { void *ctx; } SHA256_CTX;

void sha1_Init(SHA1_CTX *c);
void sha1_Update(SHA1_CTX *c, const uint8_t *d, size_t n);
void sha1_Final(SHA1_CTX *c, uint8_t out[SHA1_DIGEST_LENGTH]);

void sha256_Init(SHA256_CTX *c);
void sha256_Update(SHA256_CTX *c, const uint8_t *d, size_t n);
void sha256_Final(SHA256_CTX *c, uint8_t out[SHA256_DIGEST_LENGTH]);

typedef struct { int dummy; } ecdsa_curve;
extern const ecdsa_curve secp256k1;

int ecdsa_verify_raw_pub(const ecdsa_curve *curve,
                         const uint8_t *pub_key,
                         const uint8_t *sig,
                         const uint8_t *digest);

#endif
