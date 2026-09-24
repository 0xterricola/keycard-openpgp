#ifndef ECDSA_DER_H
#define ECDSA_DER_H

#include <stddef.h>
#include <stdint.h>

/*
 * Convert a DER-encoded ECDSA signature (SEQUENCE of two INTEGERs) into
 * the fixed-width 64-byte r||s form used by openpgp_v4_build_signature_packet.
 *
 * Keycard's SIGN returns DER; NeoPGP's raw path already returns r||s.
 */
int ecdsa_der_to_raw(
    const uint8_t *der,
    size_t der_len,
    uint8_t out[64]);

#endif
