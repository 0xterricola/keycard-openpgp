#include <string.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>

#include "shims.h"

const ecdsa_curve secp256k1 = { 0 };

static void md_init(void **c, const EVP_MD *md) {
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex(ctx, md, NULL);
  *c = ctx;
}

void sha1_Init(SHA1_CTX *c) { md_init(&c->ctx, EVP_sha1()); }
void sha1_Update(SHA1_CTX *c, const uint8_t *d, size_t n) {
  EVP_DigestUpdate((EVP_MD_CTX *)c->ctx, d, n);
}
void sha1_Final(SHA1_CTX *c, uint8_t out[SHA1_DIGEST_LENGTH]) {
  unsigned int len = 0;
  EVP_DigestFinal_ex((EVP_MD_CTX *)c->ctx, out, &len);
  EVP_MD_CTX_free((EVP_MD_CTX *)c->ctx);
}

void sha256_Init(SHA256_CTX *c) { md_init(&c->ctx, EVP_sha256()); }
void sha256_Update(SHA256_CTX *c, const uint8_t *d, size_t n) {
  EVP_DigestUpdate((EVP_MD_CTX *)c->ctx, d, n);
}
void sha256_Final(SHA256_CTX *c, uint8_t out[SHA256_DIGEST_LENGTH]) {
  unsigned int len = 0;
  EVP_DigestFinal_ex((EVP_MD_CTX *)c->ctx, out, &len);
  EVP_MD_CTX_free((EVP_MD_CTX *)c->ctx);
}

/*
 * pub_key is bare X||Y (64 bytes), sig is r||s (64 bytes),
 * digest is 32 bytes. Returns 0 on success, matching Shell.
 */
int ecdsa_verify_raw_pub(const ecdsa_curve *curve,
                         const uint8_t *pub_key,
                         const uint8_t *sig,
                         const uint8_t *digest) {
  (void)curve;

  int ok = 1;
  EC_KEY *key = EC_KEY_new_by_curve_name(NID_secp256k1);
  BIGNUM *x = BN_bin2bn(pub_key, 32, NULL);
  BIGNUM *y = BN_bin2bn(pub_key + 32, 32, NULL);
  ECDSA_SIG *s = ECDSA_SIG_new();
  BIGNUM *r = BN_bin2bn(sig, 32, NULL);
  BIGNUM *sv = BN_bin2bn(sig + 32, 32, NULL);

  if (!key || !x || !y || !s || !r || !sv) goto out;
  if (!EC_KEY_set_public_key_affine_coordinates(key, x, y)) goto out;
  if (!ECDSA_SIG_set0(s, r, sv)) goto out;
  r = NULL; sv = NULL;

  ok = (ECDSA_do_verify(digest, 32, s, key) == 1) ? 0 : 1;

out:
  EC_KEY_free(key);
  BN_free(x);
  BN_free(y);
  BN_free(r);
  BN_free(sv);
  ECDSA_SIG_free(s);
  return ok;
}
