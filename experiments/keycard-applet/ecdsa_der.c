/*
 * DER ECDSA signature -> fixed-width r||s.
 *
 *   30 <len> 02 <rlen> <r> 02 <slen> <s>
 *
 * DER INTEGERs are signed, so a value whose high bit is set carries a
 * leading 0x00. Values shorter than 32 bytes are left-padded here.
 */

#include <string.h>

#include "ecdsa_der.h"

static int copy_integer(const uint8_t *der,
                        size_t der_len,
                        size_t *pos,
                        uint8_t out[32])
{
    size_t len;
    const uint8_t *value;

    if (*pos + 2 > der_len)
        return -1;

    if (der[*pos] != 0x02)
        return -1;

    (*pos)++;

    len = der[*pos];
    (*pos)++;

    /* Long-form lengths are not expected for 256-bit integers. */
    if (len & 0x80)
        return -1;

    if (*pos + len > der_len)
        return -1;

    value = der + *pos;
    *pos += len;

    /* Strip leading zero padding. */
    while (len > 0 && value[0] == 0x00) {
        value++;
        len--;
    }

    if (len == 0 || len > 32)
        return -1;

    memset(out, 0, 32);
    memcpy(out + (32 - len), value, len);

    return 0;
}

int ecdsa_der_to_raw(const uint8_t *der, size_t der_len, uint8_t out[64])
{
    size_t pos = 0;
    size_t seq_len;

    if (!der || !out)
        return -1;

    if (der_len < 8)
        return -1;

    if (der[pos++] != 0x30)
        return -1;

    seq_len = der[pos++];

    if (seq_len & 0x80)
        return -1;

    if (pos + seq_len != der_len)
        return -1;

    if (copy_integer(der, der_len, &pos, out) != 0)
        return -1;

    if (copy_integer(der, der_len, &pos, out + 32) != 0)
        return -1;

    if (pos != der_len)
        return -1;

    return 0;
}
