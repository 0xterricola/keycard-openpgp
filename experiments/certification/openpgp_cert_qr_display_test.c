#define main sign_prompt_v4_production_main
#include "../../embedded/sama5d3/sign_prompt_v4.c"
#undef main

#include "openpgp_target.h"

int main(void)
{
    qr_cert_request_t request;
    openpgp_cert_target_t target;

    uint8_t fingerprint[OPENPGP_V4_FINGERPRINT_LEN];

    char uid[TRUSTED_CERT_MAX_UID + 1];

    lcd_init();

    clear_screen(0x0000);
    draw_text(48, 72, "SCAN CERT", 2, 0xFFFF);
    draw_text(18, 108, "SHOW QR TO CAMERA", 2, 0xFFFF);
    lcd_flush();

    if (qr_scan_cert_request("/dev/video0", &request) != 0) {
        fprintf(stderr, "Certification QR scan failed\n");
        return 1;
    }

    printf("Certification QR accepted\n");
    printf("OpenPGP bundle: %zu bytes\n",
           request.packets_len);

    /*
     * From this point onward, interpretation belongs to the trusted
     * OpenPGP layer, not the QR transport.
     */
    if (openpgp_parse_cert_target(
            request.packets,
            request.packets_len,
            &target) != 0) {
        fprintf(stderr,
                "OpenPGP certification target parse failed\n");
        return 1;
    }

    if (openpgp_v4_verify_uid_self_cert(
            target.primary_key_body,
            target.primary_key_body_len,
            target.user_id,
            target.user_id_len,
            target.self_cert_body,
            target.self_cert_body_len) != 0) {
        fprintf(stderr,
                "OpenPGP UID self-certification verification failed\n");
        return 1;
    }

    printf("UID self-certification: PASS\n");

    if (target.user_id_len == 0 ||
        target.user_id_len > TRUSTED_CERT_MAX_UID) {
        fprintf(stderr,
                "UID cannot fit trusted display policy\n");
        return 1;
    }

    /*
     * The trusted display currently supports printable ASCII.
     * Convert the parsed packet body to a C string only after
     * validating the exact bytes.
     */
    for (size_t i = 0; i < target.user_id_len; i++) {
        if (target.user_id[i] < 0x20 ||
            target.user_id[i] > 0x7e) {
            fprintf(stderr,
                    "UID contains unsupported display encoding\n");
            return 1;
        }
    }

    memcpy(uid,
           target.user_id,
           target.user_id_len);

    uid[target.user_id_len] = '\0';

    printf("UID extracted from OpenPGP packet: %s\n",
           uid);

    printf("Self-cert type: 0x%02X\n",
           target.self_cert_type);

    /*
     * show_certification_review() computes the fingerprint itself
     * from target.primary_key_body.
     */
    if (show_certification_review(
            target.primary_key_body,
            target.primary_key_body_len,
            uid,
            fingerprint) != 0) {
        fprintf(stderr,
                "Trusted certification review failed\n");
        return 1;
    }

    printf("Device-derived target fingerprint: ");

    for (size_t i = 0; i < sizeof(fingerprint); i++)
        printf("%02X", fingerprint[i]);

    printf("\n");

    printf("Waiting for physical APPROVE or REJECT...\n");

    int decision = wait_for_decision();

    if (decision < 0) {
        fprintf(stderr,
                "Could not read physical decision\n");
        return 1;
    }

    show_result(decision);

    if (decision == 0) {
        printf(
            "REJECTED - QR display test only; no signing operation\n");
        return 2;
    }

    printf(
        "APPROVED - QR display test only; no signing operation\n");

    return 0;
}
