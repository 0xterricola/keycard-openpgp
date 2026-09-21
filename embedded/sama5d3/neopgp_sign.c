#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <fcntl.h>

#include <winscard.h>
#include <openssl/sha.h>

#include "neopgp_sign.h"

static const uint8_t select_neopgp[] = {
    0x00, 0xA4, 0x04, 0x00, 0x10,
    0xD2, 0x76, 0x00, 0x01, 0x24, 0x01, 0x03, 0x04,
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00
};

static void secure_zero(void *ptr, size_t len)
{
    volatile uint8_t *p = ptr;

    while (len--)
        *p++ = 0;
}

static int connect_card(SCARDCONTEXT *ctx,
                        SCARDHANDLE *card,
                        DWORD *proto)
{
    LONG rv;
    char readers[512];
    DWORD readers_len = sizeof(readers);

    rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, ctx);
    if (rv != SCARD_S_SUCCESS)
        return -1;

    rv = SCardListReaders(*ctx, NULL, readers, &readers_len);
    if (rv != SCARD_S_SUCCESS) {
        SCardReleaseContext(*ctx);
        return -1;
    }

    rv = SCardConnect(*ctx,
                      readers,
                      SCARD_SHARE_SHARED,
                      SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
                      card,
                      proto);

    if (rv != SCARD_S_SUCCESS) {
        SCardReleaseContext(*ctx);
        return -1;
    }

    return 0;
}

static int transmit(SCARDHANDLE card,
                    DWORD proto,
                    const uint8_t *apdu,
                    size_t apdu_len,
                    uint8_t *resp,
                    DWORD *resp_len)
{
    const SCARD_IO_REQUEST *pci =
        (proto == SCARD_PROTOCOL_T0) ? SCARD_PCI_T0 : SCARD_PCI_T1;

    LONG rv = SCardTransmit(card,
                            pci,
                            apdu,
                            apdu_len,
                            NULL,
                            resp,
                            resp_len);

    return rv == SCARD_S_SUCCESS ? 0 : -1;
}

static int select_app(SCARDHANDLE card, DWORD proto)
{
    uint8_t resp[256];
    DWORD resp_len = sizeof(resp);

    if (transmit(card, proto,
                 select_neopgp, sizeof(select_neopgp),
                 resp, &resp_len) != 0)
        return -1;

    if (resp_len < 2)
        return -1;

    return (resp[resp_len - 2] == 0x90 &&
            resp[resp_len - 1] == 0x00) ? 0 : -1;
}

static int read_pin(char *pin, size_t capacity,
                    neopgp_pin_progress_fn pin_progress)
{
    int fd = open("/dev/input/event0", O_RDONLY);
    struct input_event ev;
    size_t len = 0;

    if (fd < 0)
        return -1;

    printf("Enter PIN on keypad; APPROVE submits, REJECT cancels\n");

    if (pin_progress)
        pin_progress(0);

    for (;;) {
        if (read(fd, &ev, sizeof(ev)) != sizeof(ev))
            goto fail;

        if (ev.type != EV_KEY || ev.value != 1)
            continue;

        if (ev.code == KEY_ESC) {
            secure_zero(pin, capacity);
            close(fd);
            return 1;
        }

        if (ev.code == KEY_ENTER) {
            if (len < 6)
                goto fail;

            pin[len] = '\0';
            close(fd);
            return 0;
        }

        int digit = -1;

        if (ev.code >= KEY_1 && ev.code <= KEY_9)
            digit = ev.code - KEY_1 + 1;
        else if (ev.code == KEY_0)
            digit = 0;

        if (digit >= 0) {
            if (len + 1 >= capacity)
                goto fail;

            pin[len++] = '0' + digit;

            if (pin_progress)
                pin_progress(len);
        }
    }

fail:
    secure_zero(pin, capacity);
    close(fd);
    return -1;
}

int neopgp_sign_digest(const uint8_t *digest,
                       size_t digest_len,
                       uint8_t *signature,
                       size_t *signature_len,
                       neopgp_pin_progress_fn pin_progress)
{
    SCARDCONTEXT ctx;
    SCARDHANDLE card;
    DWORD proto;

    uint8_t resp[256];
    DWORD resp_len;

    char pin[128] = {0};

    int result = -1;

    if (!digest || digest_len != SHA256_DIGEST_LENGTH) {
        fprintf(stderr, "Expected a 32-byte SHA-256 digest\n");
        return -1;
    }

    if (connect_card(&ctx, &card, &proto) != 0) {
        fprintf(stderr, "Could not connect to card\n");
        return -1;
    }

    if (select_app(card, proto) != 0) {
        fprintf(stderr, "NeoPGP SELECT failed\n");
        goto out;
    }

    int pin_result = read_pin(pin, sizeof(pin), pin_progress);

    if (pin_result == 1) {
        result = 1;
        goto out;
    }

    if (pin_result != 0)
        goto out;

    size_t pin_len = strlen(pin);

    uint8_t verify[5 + 127];

    verify[0] = 0x00;
    verify[1] = 0x20;
    verify[2] = 0x00;
    verify[3] = 0x81;
    verify[4] = (uint8_t)pin_len;

    memcpy(&verify[5], pin, pin_len);

    resp_len = sizeof(resp);

    if (transmit(card, proto,
                 verify, 5 + pin_len,
                 resp, &resp_len) != 0) {
        fprintf(stderr, "VERIFY transport failure\n");
        goto out;
    }

    secure_zero(pin, sizeof(pin));
    secure_zero(&verify[5], pin_len);

    if (resp_len < 2 ||
        resp[resp_len - 2] != 0x90 ||
        resp[resp_len - 1] != 0x00) {

        if (resp_len >= 2)
            fprintf(stderr, "VERIFY failed: %02X%02X\n",
                    resp[resp_len - 2],
                    resp[resp_len - 1]);

        goto out;
    }

    uint8_t pso[5 + SHA256_DIGEST_LENGTH + 1];

    pso[0] = 0x00;
    pso[1] = 0x2A;
    pso[2] = 0x9E;
    pso[3] = 0x9A;
    pso[4] = SHA256_DIGEST_LENGTH;

    memcpy(&pso[5], digest, SHA256_DIGEST_LENGTH);

    pso[5 + SHA256_DIGEST_LENGTH] = 0x00;

    resp_len = sizeof(resp);

    if (transmit(card, proto,
                 pso, sizeof(pso),
                 resp, &resp_len) != 0) {
        fprintf(stderr, "PSO signing transport failure\n");
        goto out;
    }

    if (resp_len < 3 ||
        resp[resp_len - 2] != 0x90 ||
        resp[resp_len - 1] != 0x00) {

        if (resp_len >= 2)
            fprintf(stderr, "PSO signing failed: %02X%02X\n",
                    resp[resp_len - 2],
                    resp[resp_len - 1]);

        goto out;
    }

    size_t sig_len = resp_len - 2;

    if (sig_len > *signature_len) {
        fprintf(stderr, "Signature buffer too small\n");
        goto out;
    }

    memcpy(signature, resp, sig_len);
    *signature_len = sig_len;

    result = 0;

out:
    secure_zero(pin, sizeof(pin));

    SCardDisconnect(card, SCARD_LEAVE_CARD);
    SCardReleaseContext(ctx);

    return result;
}

int neopgp_sign_message(const char *message,
                        uint8_t *signature,
                        size_t *signature_len,
                        neopgp_pin_progress_fn pin_progress)
{
    uint8_t digest[SHA256_DIGEST_LENGTH];

    if (!message)
        return -1;

    SHA256((const unsigned char *)message,
           strlen(message),
           digest);

    int result = neopgp_sign_digest(digest,
                                    sizeof(digest),
                                    signature,
                                    signature_len,
                                    pin_progress);

    secure_zero(digest, sizeof(digest));

    return result;
}

int neopgp_signature_counter(uint32_t *counter)
{
    SCARDCONTEXT ctx;
    SCARDHANDLE card;
    DWORD proto;

    uint8_t resp[256];
    DWORD resp_len = sizeof(resp);

    const uint8_t get_counter[] = {
        0x00, 0xCA, 0x00, 0x7A, 0x00
    };

    int result = -1;

    if (connect_card(&ctx, &card, &proto) != 0)
        return -1;

    if (select_app(card, proto) != 0)
        goto out;

    if (transmit(card, proto,
                 get_counter, sizeof(get_counter),
                 resp, &resp_len) != 0)
        goto out;

    if (resp_len < 7 ||
        resp[resp_len - 2] != 0x90 ||
        resp[resp_len - 1] != 0x00)
        goto out;

    for (DWORD i = 0; i + 4 < resp_len - 2; i++) {
        if (resp[i] == 0x93 && resp[i + 1] == 0x03) {
            *counter =
                ((uint32_t)resp[i + 2] << 16) |
                ((uint32_t)resp[i + 3] << 8) |
                ((uint32_t)resp[i + 4]);

            result = 0;
            break;
        }
    }

out:
    SCardDisconnect(card, SCARD_LEAVE_CARD);
    SCardReleaseContext(ctx);

    return result;
}
