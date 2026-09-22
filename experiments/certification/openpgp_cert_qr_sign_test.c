#define main sign_prompt_v4_production_main
#include "../../embedded/sama5d3/sign_prompt_v4.c"
#undef main

#include "openpgp_target.h"

#include <time.h>

static int hex_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';

    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;

    return -1;
}

static int decode_fingerprint(
    const char hex[40],
    uint8_t fingerprint[OPENPGP_V4_FINGERPRINT_LEN])
{
    for (size_t i = 0; i < OPENPGP_V4_FINGERPRINT_LEN; i++) {
        int hi = hex_value(hex[i * 2]);
        int lo = hex_value(hex[i * 2 + 1]);

        if (hi < 0 || lo < 0)
            return -1;

        fingerprint[i] = (uint8_t)((hi << 4) | lo);
    }

    return 0;
}

static uint32_t read_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) |
           (uint32_t)p[3];
}

static int write_binary(
    const char *path,
    const uint8_t *data,
    size_t len)
{
    FILE *f = fopen(path, "wb");

    if (!f)
        return -1;

    if (fwrite(data, 1, len, f) != len) {
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}

int main(void)
{
    qr_cert_request_t request;
    openpgp_cert_target_t target;

    char uid[TRUSTED_CERT_MAX_UID + 1];

    uint8_t target_fingerprint[OPENPGP_V4_FINGERPRINT_LEN];

    char signer_fp_hex[40];
    uint8_t signer_fingerprint[OPENPGP_V4_FINGERPRINT_LEN];
    uint8_t signer_key_id[8];

    uint8_t certification_data[2048];
    size_t certification_data_len = 0;

    uint8_t sig_fields[OPENPGP_V4_SIG_FIELDS_LEN];
    size_t sig_fields_len = 0;

    uint8_t digest[OPENPGP_SHA256_LEN];

    uint8_t raw_signature[OPENPGP_RAW_ECDSA_LEN];
    size_t raw_signature_len = sizeof(raw_signature);

    uint8_t signature_packet[256];
    size_t signature_packet_len = 0;

    uint32_t counter_before = 0;
    uint32_t counter_after = 0;

    uint32_t target_creation_time;
    uint32_t creation_time;

    time_t now;

    lcd_init();

    clear_screen(0x0000);
    draw_text(48, 72, "SCAN CERT", 2, 0xFFFF);
    draw_text(18, 108, "SHOW QR TO CAMERA", 2, 0xFFFF);
    lcd_flush();

    /*
     * QR transport provides only opaque OpenPGP packet bytes.
     */
    if (qr_scan_cert_request("/dev/video0", &request) != 0) {
        fprintf(stderr, "Certification QR scan failed\n");
        return 1;
    }

    printf("Certification QR accepted\n");
    printf("OpenPGP bundle: %zu bytes\n", request.packets_len);

    /*
     * Trusted OpenPGP layer interprets those exact bytes.
     */
    if (openpgp_parse_cert_target(
            request.packets,
            request.packets_len,
            &target) != 0) {
        fprintf(stderr, "OpenPGP target parse failed\n");
        return 1;
    }

    /*
     * Do not display or certify an unauthenticated UID.
     */
    if (openpgp_v4_verify_uid_self_cert(
            target.primary_key_body,
            target.primary_key_body_len,
            target.user_id,
            target.user_id_len,
            target.self_cert_body,
            target.self_cert_body_len) != 0) {
        fprintf(stderr, "UID self-certification verification failed\n");
        return 1;
    }

    printf("UID self-certification: PASS\n");

    if (target.user_id_len == 0 ||
        target.user_id_len > TRUSTED_CERT_MAX_UID) {
        fprintf(stderr, "UID cannot fit trusted display policy\n");
        return 1;
    }

    for (size_t i = 0; i < target.user_id_len; i++) {
        if (target.user_id[i] < 0x20 ||
            target.user_id[i] > 0x7e) {
            fprintf(stderr, "UID has unsupported display encoding\n");
            return 1;
        }
    }

    memcpy(uid, target.user_id, target.user_id_len);
    uid[target.user_id_len] = '\0';

    printf("UID extracted from OpenPGP packet: %s\n", uid);

    /*
     * Read signing identity from the actual NeoPGP card.
     * The host request supplies no signer fingerprint or key ID.
     */
    if (neopgp_fingerprint(signer_fp_hex) != 0) {
        fprintf(stderr, "Could not read NeoPGP signer fingerprint\n");
        return 1;
    }

    if (decode_fingerprint(
            signer_fp_hex,
            signer_fingerprint) != 0) {
        fprintf(stderr, "Invalid NeoPGP fingerprint encoding\n");
        return 1;
    }

    memcpy(
        signer_key_id,
        signer_fingerprint + OPENPGP_V4_FINGERPRINT_LEN - 8,
        sizeof(signer_key_id));

    printf("NeoPGP signer fingerprint: ");
    for (size_t i = 0;
         i < sizeof(signer_fingerprint);
         i++)
        printf("%02X", signer_fingerprint[i]);
    printf("\n");

    if (neopgp_signature_counter(&counter_before) != 0) {
        fprintf(stderr, "Could not read signature counter\n");
        return 1;
    }

    printf("Signature counter before: %u\n", counter_before);

    /*
     * Fingerprint shown to the human is derived locally from the
     * same primary-key body that will be certified.
     */
    if (show_certification_review(
            target.primary_key_body,
            target.primary_key_body_len,
            uid,
            target_fingerprint) != 0) {
        fprintf(stderr, "Trusted review failed\n");
        return 1;
    }

    printf("Device-derived target fingerprint: ");

    for (size_t i = 0;
         i < sizeof(target_fingerprint);
         i++)
        printf("%02X", target_fingerprint[i]);

    printf("\n");

    printf("Waiting for physical APPROVE or REJECT...\n");

    int decision = wait_for_decision();

    if (decision < 0)
        return 1;

    /*
     * Rejection must occur before any signing operation.
     */
    if (decision == 0) {
        show_result(0);

        if (neopgp_signature_counter(&counter_after) != 0)
            return 1;

        printf("Signature counter after:  %u\n", counter_after);

        if (counter_after != counter_before) {
            fprintf(stderr,
                    "FAIL: signature counter changed after REJECT\n");
            return 1;
        }

        printf("REJECTED - no signing APDU executed\n");
        printf("Reject counter test: PASS\n");
        return 2;
    }

    printf("Physical approval received\n");

    /*
     * Signature creation time comes from the device-local clock.
     * The QR does not control it.
     */
    if (target.primary_key_body_len < 5)
        return 1;

    target_creation_time =
        read_be32(target.primary_key_body + 1);

    now = time(NULL);

    if (now < 0 ||
        (uint64_t)now > UINT32_MAX ||
        (uint64_t)now < target_creation_time) {
        fprintf(stderr,
                "Device clock is invalid for this target; refusing to sign\n");
        return 1;
    }

    creation_time = (uint32_t)now;

    printf("Device certification timestamp: %u\n",
           creation_time);

    /*
     * Build the certification preimage from the exact parsed key
     * body and UID that were verified and reviewed.
     */
    if (openpgp_v4_build_certification_data(
            target.primary_key_body,
            target.primary_key_body_len,
            target.user_id,
            target.user_id_len,
            certification_data,
            sizeof(certification_data),
            &certification_data_len) != 0) {
        fprintf(stderr,
                "Could not construct certification data\n");
        return 1;
    }

    /*
     * The trusted operation chooses 0x10 generic certification.
     */
    if (openpgp_v4_build_sig_fields_for_type(
            0x10,
            signer_fingerprint,
            creation_time,
            sig_fields,
            sizeof(sig_fields),
            &sig_fields_len) != 0) {
        fprintf(stderr,
                "Could not construct certification fields\n");
        return 1;
    }

    if (openpgp_v4_digest(
            certification_data,
            certification_data_len,
            sig_fields,
            sig_fields_len,
            digest) != 0) {
        fprintf(stderr,
                "Could not derive certification digest\n");
        return 1;
    }

    printf("Device-derived certification digest: ");

    for (size_t i = 0; i < sizeof(digest); i++)
        printf("%02x", digest[i]);

    printf("\n");

    /*
     * First point at which the private-key operation is possible.
     * Physical approval has already occurred.
     */
    int sign_result = neopgp_sign_digest(
        digest,
        sizeof(digest),
        raw_signature,
        &raw_signature_len,
        show_pin_entry);

    if (sign_result == 1) {
        printf("PIN ENTRY CANCELLED\n");
        return 2;
    }

    if (sign_result != 0) {
        fprintf(stderr, "NeoPGP hardware signing failed\n");
        return 1;
    }

    if (raw_signature_len != OPENPGP_RAW_ECDSA_LEN) {
        fprintf(stderr,
                "Unexpected raw signature length: %zu\n",
                raw_signature_len);
        return 1;
    }

    if (openpgp_v4_build_signature_packet(
            sig_fields,
            sig_fields_len,
            digest,
            raw_signature,
            signer_key_id,
            signature_packet,
            sizeof(signature_packet),
            &signature_packet_len) != 0) {
        fprintf(stderr,
                "Could not construct OpenPGP signature packet\n");
        return 1;
    }

    if (write_binary(
            "/tmp/openpgp-approved-cert.sig",
            signature_packet,
            signature_packet_len) != 0) {
        fprintf(stderr,
                "Could not write certification packet\n");
        return 1;
    }

    if (neopgp_signature_counter(&counter_after) != 0)
        return 1;

    if (counter_after != counter_before + 1) {
        fprintf(stderr,
                "FAIL: signature counter did not increase exactly once\n");
        return 1;
    }

    show_result(1);

    printf("Signature counter after:  %u\n", counter_after);
    printf("Hardware certification: PASS\n");
    printf("Signature counter test: PASS\n");
    printf("OpenPGP packet: %zu bytes\n", signature_packet_len);
    printf("Wrote /tmp/openpgp-approved-cert.sig\n");

    /*
     * Return the standard OpenPGP Signature packet over the
     * disconnected QR transport.
     */
    usleep(600000);

    if (show_certification_response_qr(
            signature_packet,
            signature_packet_len) != 0) {
        fprintf(stderr,
                "Could not render certification response QR\n");
        return 1;
    }

    printf("Certification response QR: PASS\n");

    return 0;
}
