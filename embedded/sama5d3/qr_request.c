#include "qr_request.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <zbar.h>

#define REQUEST_PREFIX "KC1|OP=PGP_SIGN|MSG="

static int message_is_safe(const unsigned char *msg, size_t len)
{
    if (len == 0 || len > QR_REQUEST_MAX_MESSAGE)
        return 0;

    /*
     * Prototype rule:
     * only printable ASCII.
     *
     * This prevents hidden/control bytes from being signed while the
     * trusted display shows something different.
     */
    for (size_t i = 0; i < len; i++) {
        if (msg[i] < 0x20 || msg[i] > 0x7e)
            return 0;
    }

    return 1;
}

int qr_scan_sign_request(const char *device, qr_sign_request_t *request)
{
    const char prefix[] = REQUEST_PREFIX;
    const size_t prefix_len = sizeof(prefix) - 1;

    zbar_processor_t *proc = zbar_processor_create(0);
    if (!proc) {
        fprintf(stderr, "zbar_processor_create failed\n");
        return -1;
    }

    if (zbar_processor_request_size(proc, 640, 480) != 0)
        fprintf(stderr, "warning: could not request 640x480\n");

    if (zbar_processor_init(proc, device, 0) != 0) {
        fprintf(stderr, "zbar_processor_init failed for %s\n", device);
        zbar_processor_destroy(proc);
        return -1;
    }

    if (zbar_processor_set_active(proc, 1) != 0) {
        fprintf(stderr, "could not activate camera\n");
        zbar_processor_destroy(proc);
        return -1;
    }

    printf("Waiting for KC1 signing request on %s...\n", device);

    for (;;) {
        int rc = zbar_process_one(proc, 1000);

        if (rc < 0) {
            fprintf(stderr, "zbar_process_one failed\n");
            zbar_processor_set_active(proc, 0);
            zbar_processor_destroy(proc);
            return -1;
        }

        if (rc == 0)
            continue;

        const zbar_symbol_set_t *symbols =
            zbar_processor_get_results(proc);

        const zbar_symbol_t *sym =
            zbar_symbol_set_first_symbol(symbols);

        for (; sym; sym = zbar_symbol_next(sym)) {
            if (zbar_symbol_get_type(sym) != ZBAR_QRCODE)
                continue;

            const unsigned char *data =
                (const unsigned char *)zbar_symbol_get_data(sym);

            size_t data_len =
                zbar_symbol_get_data_length(sym);

            if (data_len < prefix_len ||
                memcmp(data, prefix, prefix_len) != 0) {
                printf("Ignored QR: not a KC1 PGP signing request\n");
                continue;
            }

            const unsigned char *msg = data + prefix_len;
            size_t msg_len = data_len - prefix_len;

            if (!message_is_safe(msg, msg_len)) {
                printf("Ignored QR: invalid message encoding/length\n");
                continue;
            }

            memcpy(request->message, msg, msg_len);
            request->message[msg_len] = '\0';

            zbar_processor_set_active(proc, 0);
            zbar_processor_destroy(proc);

            return 0;
        }
    }
}
