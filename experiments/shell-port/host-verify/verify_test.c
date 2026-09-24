#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openpgp/openpgp_packet.h"
#include "openpgp/openpgp_v4.h"

static const uint8_t expected_fp[20] = {
  0xa1, 0x26, 0x5c, 0x68, 0x9e, 0xc7, 0xc6, 0x00, 0x18, 0xc0,
  0xaf, 0xb0, 0x23, 0xa9, 0x2d, 0x87, 0xd6, 0x3f, 0x7e, 0x7c
};

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("usage: %s <bundle.gpg>\n", argv[0]);
    return 2;
  }

  FILE *f = fopen(argv[1], "rb");
  if (!f) { perror("open"); return 1; }

  uint8_t buf[4096];
  size_t len = fread(buf, 1, sizeof(buf), f);
  fclose(f);

  printf("bundle: %zu bytes\n", len);

  openpgp_cert_target_t target;
  if (openpgp_parse_cert_target(buf, len, &target) != 0) {
    printf("FAIL: parse\n");
    return 1;
  }
  printf("PASS: parsed three packets\n");

  char uid[256];
  size_t uid_len = target.user_id_len < 255 ? target.user_id_len : 255;
  memcpy(uid, target.user_id, uid_len);
  uid[uid_len] = 0;
  printf("uid: %s\n", uid);

  uint8_t fp[OPENPGP_V4_FINGERPRINT_LEN];
  if (openpgp_v4_primary_key_fingerprint(
        target.primary_key_body, target.primary_key_body_len, fp) != 0) {
    printf("FAIL: fingerprint\n");
    return 1;
  }

  printf("fingerprint: ");
  for (size_t i = 0; i < sizeof(fp); i++) printf("%02x", fp[i]);
  printf("\n");

  if (memcmp(fp, expected_fp, sizeof(fp)) != 0) {
    printf("FAIL: fingerprint mismatch\n");
    return 1;
  }
  printf("PASS: fingerprint matches A1265C68...\n");

  int rc = openpgp_v4_verify_uid_self_cert(
      target.primary_key_body, target.primary_key_body_len,
      target.user_id, target.user_id_len,
      target.self_cert_body, target.self_cert_body_len);

  if (rc != 0) {
    printf("FAIL: self-cert verification\n");
    return 1;
  }
  printf("PASS: secp256k1 self-certification verified\n");

  return 0;
}
