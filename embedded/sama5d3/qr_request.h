#ifndef QR_REQUEST_H
#define QR_REQUEST_H

#define QR_REQUEST_MAX_MESSAGE 160

typedef struct {
    char message[QR_REQUEST_MAX_MESSAGE + 1];
} qr_sign_request_t;

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

#endif
