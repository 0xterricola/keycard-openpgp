#ifndef QR_REQUEST_H
#define QR_REQUEST_H

#include <stddef.h>
#include <stdint.h>

#define QR_REQUEST_MAX_MESSAGE 160

#define QR_CERT_MAX_BUNDLE 1024

typedef struct {
    char message[QR_REQUEST_MAX_MESSAGE + 1];
} qr_sign_request_t;

typedef struct {
    uint8_t packets[QR_CERT_MAX_BUNDLE];
    size_t packets_len;
} qr_cert_request_t;

/*
 * Blocks until a valid KC1 PGP signing request is scanned.
 *
 * Accepted format:
 *   KC1|OP=PGP_SIGN|MSG=<printable ASCII message>
 *
 * Returns:
 *   0  success
 *  -1 camera/ZBar failure
 */
int qr_scan_sign_request(const char *device, qr_sign_request_t *request);

/*
 * Blocks until a valid KC1 OpenPGP certification request is scanned.
 *
 * Accepted format:
 *   KC1|OP=PGP_CERT|CERT=<hex OpenPGP packet bundle>
 *
 * The QR layer transports opaque OpenPGP bytes only.
 * UID, fingerprint, and signing digest are derived by the trusted
 * OpenPGP layer from those exact bytes.
 */
int qr_scan_cert_request(const char *device, qr_cert_request_t *request);

#endif
