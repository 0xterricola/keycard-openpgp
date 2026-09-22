/*
 * Physical trusted-display fixture.
 *
 * This deliberately includes the current SAMA application implementation
 * so the experiment exercises the exact renderer/review functions that the
 * real flow will use.
 *
 * The production main() is renamed and discarded by the linker.
 */
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

static const char target_uid[] =
    "Thurin Labs <hello@thurin.id>";

int main(void)
{
    uint8_t fingerprint[OPENPGP_V4_FINGERPRINT_LEN];

    lcd_init();

    if (show_certification_review(
            target_primary_key_body,
            sizeof(target_primary_key_body),
            target_uid,
            fingerprint) != 0) {
        fprintf(stderr, "Certification review rendering failed\n");
        return 1;
    }

    printf("Trusted review target UID: %s\n", target_uid);

    printf("Locally derived target fingerprint: ");
    for (size_t i = 0; i < sizeof(fingerprint); i++)
        printf("%02X", fingerprint[i]);
    printf("\n");

    printf("Waiting for physical APPROVE or REJECT...\n");

    int decision = wait_for_decision();

    if (decision < 0) {
        fprintf(stderr, "Could not read physical decision\n");
        return 1;
    }

    show_result(decision);

    if (decision == 0) {
        printf("REJECTED - display test only; no signing operation exists in this fixture\n");
        return 2;
    }

    printf("APPROVED - display test only; no signing operation exists in this fixture\n");
    return 0;
}
