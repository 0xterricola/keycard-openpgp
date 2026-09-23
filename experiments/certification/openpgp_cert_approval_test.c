#define main sign_prompt_v4_production_main
#include "../../embedded/sama5d3/sign_prompt_v4.c"
#undef main

static const uint8_t target_primary_key_body[] = {
    0x04, 0x6a, 0xa5, 0x86, 0x0c, 0x16, 0x09, 0x2b,
    0x06, 0x01, 0x04, 0x01, 0xda, 0x47, 0x0f, 0x01,
    0x01, 0x07, 0x40, 0xe0, 0xad, 0x14, 0x15, 0x77,
    0x9b, 0xb4, 0xe0, 0x77, 0xff, 0x19, 0x93, 0xa7,
    0x65, 0xda, 0x2d, 0xac, 0x53, 0x95, 0xa2, 0xcb,
    0x4b, 0xef, 0xdc, 0xb4, 0xb1, 0xed, 0x81, 0x07,
    0x67, 0x88, 0xc8
};

static const uint8_t target_uid[] =
    "Thurin Labs <hello@thurin.id>";

static const uint8_t signer_fingerprint[OPENPGP_V4_FINGERPRINT_LEN] = {
    0x31, 0xce, 0x69, 0xd6, 0x6a,
    0x5e, 0x9d, 0xe0, 0xf9, 0x77,
    0xb5, 0x9c, 0x79, 0xbb, 0x39,
    0x14, 0x97, 0xe8, 0xe6, 0xd4
};

static const uint8_t signer_key_id[8] = {
    0x79, 0xbb, 0x39, 0x14,
    0x97, 0xe8, 0xe6, 0xd4
};

/* Deterministic checkpoint timestamp. */
static const uint32_t creation_time = 0x6aac6f88;

static int write_binary(const char *path,
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
    uint8_t target_fingerprint[OPENPGP_V4_FINGERPRINT_LEN];

    uint8_t certification_data[128];
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

    lcd_init();

    if (neopgp_signature_counter(&counter_before) != 0) {
        fprintf(stderr, "Could not read signature counter before review\n");
        return 1;
    }

    printf("Signature counter before: %u\n", counter_before);

    if (show_certification_review(
            target_primary_key_body,
            sizeof(target_primary_key_body),
            (const char *)target_uid,
            target_fingerprint) != 0) {
        fprintf(stderr, "Trusted certification review failed\n");
        return 1;
    }

    printf("Locally derived target fingerprint: ");
    for (size_t i = 0; i < sizeof(target_fingerprint); i++)
        printf("%02X", target_fingerprint[i]);
    printf("\n");

    printf("Waiting for physical APPROVE or REJECT...\n");

    int decision = wait_for_decision();

    if (decision < 0) {
        fprintf(stderr, "Could not read physical decision\n");
        return 1;
    }

    if (decision == 0) {
        show_result(0);

        if (neopgp_signature_counter(&counter_after) != 0) {
            fprintf(stderr, "Could not read signature counter after rejection\n");
            return 1;
        }

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

    if (openpgp_v4_build_certification_data(
            target_primary_key_body,
            sizeof(target_primary_key_body),
            target_uid,
            sizeof(target_uid) - 1,
            certification_data,
            sizeof(certification_data),
            &certification_data_len) != 0) {
        fprintf(stderr, "Could not construct certification data\n");
        return 1;
    }

    if (openpgp_v4_build_sig_fields_for_type(
            0x10,
            signer_fingerprint,
            creation_time,
            sig_fields,
            sizeof(sig_fields),
            &sig_fields_len) != 0) {
        fprintf(stderr, "Could not construct certification fields\n");
        return 1;
    }

    if (openpgp_v4_digest(
            certification_data,
            certification_data_len,
            sig_fields,
            sig_fields_len,
            digest) != 0) {
        fprintf(stderr, "Could not derive certification digest\n");
        return 1;
    }

    printf("Certification digest: ");
    for (size_t i = 0; i < sizeof(digest); i++)
        printf("%02x", digest[i]);
    printf("\n");

    /*
     * This is the first point at which a signing operation is possible.
     * Physical approval has already occurred.
     *
     * PIN entry happens locally on the trusted keypad.
     */
    int sign_result = neopgp_sign_digest(
        digest,
        sizeof(digest),
        raw_signature,
        &raw_signature_len,
        show_pin_entry);

    if (sign_result == 1) {
        if (neopgp_signature_counter(&counter_after) == 0)
            printf("Signature counter after PIN cancel: %u\n",
                   counter_after);

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
        fprintf(stderr, "Could not construct OpenPGP signature packet\n");
        return 1;
    }

    if (write_binary(
            "/tmp/thurin-labs-approved-cert.sig",
            signature_packet,
            signature_packet_len) != 0) {
        fprintf(stderr, "Could not write certification packet\n");
        return 1;
    }

    if (neopgp_signature_counter(&counter_after) != 0) {
        fprintf(stderr, "Could not read signature counter after signing\n");
        return 1;
    }

    show_result(1);

    printf("Signature counter after:  %u\n", counter_after);

    if (counter_after != counter_before + 1) {
        fprintf(stderr,
                "FAIL: expected signature counter to increase by exactly one\n");
        return 1;
    }

    printf("Hardware certification: PASS\n");
    printf("Signature counter test: PASS\n");
    printf("OpenPGP packet: %zu bytes\n", signature_packet_len);
    printf("Wrote /tmp/thurin-labs-approved-cert.sig\n");

    return 0;
}
