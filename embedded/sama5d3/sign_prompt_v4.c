#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/spi/spidev.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <winscard.h>
#include <qrencode.h>
#include "neopgp_sign.h"
#include "openpgp_v4.h"
#include "qr_request.h"

#define TRUSTED_DISPLAY_MAX_MESSAGE 20
#define TRUSTED_CERT_MAX_UID 40

#define W 240
#define H 240

#define KEY_APPROVE KEY_ENTER
#define KEY_REJECT  KEY_ESC

static int spi_fd = -1;
static int dc_fd = -1;
static uint16_t fb[W * H];

static const uint8_t digits[10][7] = {
    {14,17,19,21,25,17,14},
    {4,12,4,4,4,4,14},
    {14,17,1,2,4,8,31},
    {30,1,1,14,1,1,30},
    {2,6,10,18,31,2,2},
    {31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14},
    {31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14},
    {14,17,17,15,1,1,14}
};

static const uint8_t upper[26][7] = {
    {14,17,17,31,17,17,17},  /* A */
    {30,17,17,30,17,17,30},  /* B */
    {15,16,16,16,16,16,15},  /* C */
    {30,17,17,17,17,17,30},  /* D */
    {31,16,16,30,16,16,31},  /* E */
    {31,16,16,30,16,16,16},  /* F */
    {14,17,16,23,17,17,14},  /* G */
    {17,17,17,31,17,17,17},  /* H */
    {31,4,4,4,4,4,31},       /* I */
    {7,2,2,2,18,18,12},      /* J */
    {17,18,20,24,20,18,17},  /* K */
    {16,16,16,16,16,16,31},  /* L */
    {17,27,21,21,17,17,17},  /* M */
    {17,25,21,19,17,17,17},  /* N */
    {14,17,17,17,17,17,14},  /* O */
    {30,17,17,30,16,16,16},  /* P */
    {14,17,17,17,21,18,13},  /* Q */
    {30,17,17,30,20,18,17},  /* R */
    {15,16,16,14,1,1,30},    /* S */
    {31,4,4,4,4,4,4},        /* T */
    {17,17,17,17,17,17,14},  /* U */
    {17,17,17,17,17,10,4},   /* V */
    {17,17,17,21,21,21,10},  /* W */
    {17,17,10,4,10,17,17},   /* X */
    {17,17,10,4,4,4,4},      /* Y */
    {31,1,2,4,8,16,31}       /* Z */
};

static const uint8_t lower[26][7] = {
    {0,14,1,15,17,19,13},       /* a */
    {16,16,30,17,17,17,30},     /* b */
    {0,14,17,16,16,17,14},      /* c */
    {1,1,15,17,17,17,15},       /* d */
    {0,14,17,31,16,16,14},      /* e */
    {6,9,8,28,8,8,8},           /* f */
    {0,15,17,17,15,1,14},       /* g */
    {16,16,30,17,17,17,17},     /* h */
    {4,0,12,4,4,4,14},          /* i */
    {2,0,6,2,2,18,12},          /* j */
    {16,16,18,20,24,20,18},     /* k */
    {12,4,4,4,4,4,14},          /* l */
    {0,0,26,21,21,21,21},       /* m */
    {0,0,30,17,17,17,17},       /* n */
    {0,14,17,17,17,17,14},      /* o */
    {0,30,17,17,30,16,16},      /* p */
    {0,15,17,17,15,1,1},        /* q */
    {0,0,22,25,16,16,16},       /* r */
    {0,15,16,14,1,1,30},        /* s */
    {8,8,28,8,8,9,6},           /* t */
    {0,0,17,17,17,19,13},       /* u */
    {0,0,17,17,17,10,4},        /* v */
    {0,0,17,17,21,21,10},       /* w */
    {0,0,17,10,4,10,17},        /* x */
    {0,17,17,17,15,1,14},       /* y */
    {0,0,31,2,4,8,31}           /* z */
};

static const uint8_t glyph_space[7] = {0,0,0,0,0,0,0};
static const uint8_t glyph_dot[7]   = {0,0,0,0,0,12,12};
static const uint8_t glyph_dash[7]  = {0,0,0,31,0,0,0};
static const uint8_t glyph_colon[7] = {0,12,12,0,12,12,0};
static const uint8_t glyph_at[7]    = {14,17,23,21,23,16,14};
static const uint8_t glyph_lt[7]    = {2,4,8,16,8,4,2};
static const uint8_t glyph_gt[7]    = {8,4,2,1,2,4,8};
static const uint8_t glyph_plus[7]  = {0,4,4,31,4,4,0};

static const uint8_t *glyph(char c)
{
    if (c >= 'A' && c <= 'Z')
        return upper[c - 'A'];

    if (c >= 'a' && c <= 'z')
        return lower[c - 'a'];

    if (c >= '0' && c <= '9')
        return digits[c - '0'];

    switch (c) {
    case ' ': return glyph_space;
    case '.': return glyph_dot;
    case '-': return glyph_dash;
    case ':': return glyph_colon;
    case '@': return glyph_at;
    case '<': return glyph_lt;
    case '>': return glyph_gt;
    case '+': return glyph_plus;
    default:  return NULL;
    }
}

static int text_is_renderable(const char *s)
{
    if (!s)
        return 0;

    while (*s) {
        if (!glyph(*s))
            return 0;
        s++;
    }

    return 1;
}

static void pixel(int x, int y, uint16_t color)
{
    if (x >= 0 && x < W && y >= 0 && y < H)
        fb[y * W + x] = color;
}

static void clear_screen(uint16_t color)
{
    for (int i = 0; i < W * H; i++)
        fb[i] = color;
}

static void draw_char(int x, int y, char c, int scale, uint16_t color)
{
    const uint8_t *g = glyph(c);

    if (!g)
        return;

    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (g[row] & (1 << (4 - col))) {
                for (int yy = 0; yy < scale; yy++)
                    for (int xx = 0; xx < scale; xx++)
                        pixel(x + col * scale + xx,
                              y + row * scale + yy,
                              color);
            }
        }
    }
}

static void draw_text(int x, int y, const char *s, int scale, uint16_t color)
{
    while (*s) {
        draw_char(x, y, *s++, scale, color);
        x += 6 * scale;
    }
}

static void write_file(const char *path, const char *value)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0)
        return;

    write(fd, value, strlen(value));
    close(fd);
}

static void setup_dc(void)
{
    if (access("/sys/class/gpio/pioC28/value", F_OK) != 0) {
        write_file("/sys/class/gpio/export", "92");
        usleep(100000);
    }

    write_file("/sys/class/gpio/pioC28/direction", "out");

    dc_fd = open("/sys/class/gpio/pioC28/value", O_WRONLY);
    if (dc_fd < 0) {
        perror("open D/C gpio");
        exit(1);
    }
}

static void dc(int value)
{
    lseek(dc_fd, 0, SEEK_SET);
    write(dc_fd, value ? "1" : "0", 1);
}

static void spi_send(const void *data, size_t len)
{
    const uint8_t *p = data;

    while (len) {
        size_t n = len > 4096 ? 4096 : len;
        ssize_t r = write(spi_fd, p, n);

        if (r <= 0) {
            perror("SPI write");
            exit(1);
        }

        p += r;
        len -= r;
    }
}

static void cmd(uint8_t c)
{
    dc(0);
    spi_send(&c, 1);
}

static void data(const void *buf, size_t len)
{
    dc(1);
    spi_send(buf, len);
}

static void lcd_init(void)
{
    spi_fd = open("/dev/spidev1.0", O_WRONLY);
    if (spi_fd < 0) {
        perror("open /dev/spidev1.0");
        exit(1);
    }

    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t hz = 1000000;

    ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
    ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &hz);

    setup_dc();

    cmd(0x01);
    usleep(150000);

    cmd(0x11);
    usleep(120000);

    cmd(0x3A);
    {
        uint8_t v = 0x55;
        data(&v, 1);
    }

    cmd(0x36);
    {
        uint8_t v = 0xC0;
        data(&v, 1);
    }

    cmd(0x21);
    cmd(0x13);
    cmd(0x29);

    usleep(20000);
}

static void lcd_flush(void)
{
    uint8_t col[] = {0x00,0x00,0x00,0xEF};
    uint8_t row[] = {0x00,0x50,0x01,0x3F};

    cmd(0x2A);
    data(col, sizeof(col));

    cmd(0x2B);
    data(row, sizeof(row));

    cmd(0x2C);

    static uint8_t out[W * H * 2];

    for (int i = 0; i < W * H; i++) {
        out[i * 2]     = fb[i] >> 8;
        out[i * 2 + 1] = fb[i] & 0xff;
    }

    data(out, sizeof(out));
}

static int neopgp_fingerprint(char out[40])
{
    SCARDCONTEXT ctx;
    SCARDHANDLE card;
    DWORD proto;
    LONG rv;

    rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &ctx);
    if (rv != SCARD_S_SUCCESS)
        return -1;

    char readers[512];
    DWORD readers_len = sizeof(readers);

    rv = SCardListReaders(ctx, NULL, readers, &readers_len);
    if (rv != SCARD_S_SUCCESS) {
        SCardReleaseContext(ctx);
        return -1;
    }

    rv = SCardConnect(ctx, readers, SCARD_SHARE_SHARED,
                      SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
                      &card, &proto);

    if (rv != SCARD_S_SUCCESS) {
        SCardReleaseContext(ctx);
        return -1;
    }

    const SCARD_IO_REQUEST *pci =
        (proto == SCARD_PROTOCOL_T0) ? SCARD_PCI_T0 : SCARD_PCI_T1;

    const uint8_t select[] = {
        0x00,0xA4,0x04,0x00,0x10,
        0xD2,0x76,0x00,0x01,0x24,0x01,0x03,0x04,
        0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00
    };

    uint8_t resp[256];
    DWORD resp_len = sizeof(resp);

    rv = SCardTransmit(card, pci,
                       select, sizeof(select),
                       NULL, resp, &resp_len);

    if (rv != SCARD_S_SUCCESS ||
        resp_len < 2 ||
        resp[resp_len - 2] != 0x90 ||
        resp[resp_len - 1] != 0x00) {
        SCardDisconnect(card, SCARD_LEAVE_CARD);
        SCardReleaseContext(ctx);
        return -1;
    }

    const uint8_t get_fp[] = {0x00,0xCA,0x00,0xC5,0x00};
    resp_len = sizeof(resp);

    rv = SCardTransmit(card, pci,
                       get_fp, sizeof(get_fp),
                       NULL, resp, &resp_len);

    if (rv != SCARD_S_SUCCESS ||
        resp_len < 22 ||
        resp[resp_len - 2] != 0x90 ||
        resp[resp_len - 1] != 0x00) {
        SCardDisconnect(card, SCARD_LEAVE_CARD);
        SCardReleaseContext(ctx);
        return -1;
    }

    static const char hex[] = "0123456789ABCDEF";

    for (int i = 0; i < 20; i++) {
        out[i * 2]     = hex[resp[i] >> 4];
        out[i * 2 + 1] = hex[resp[i] & 0x0F];
    }

    SCardDisconnect(card, SCARD_LEAVE_CARD);
    SCardReleaseContext(ctx);

    return 0;
}

static void show_prompt(const char *fp, const char *message)
{
    const uint16_t BLACK = 0x0000;
    const uint16_t WHITE = 0xFFFF;
    const uint16_t GREEN = 0x07E0;
    const uint16_t RED   = 0xF800;

    char shortfp[20];

    memcpy(shortfp, fp, 8);
    memcpy(shortfp + 8, "...", 3);
    memcpy(shortfp + 11, fp + 32, 8);
    shortfp[19] = '\0';

    clear_screen(BLACK);

    draw_text(30, 18,  "OPENPGP SIGN",      2, WHITE);
    draw_text(48, 48,  "NEOPGP CARD",       2, WHITE);
    draw_text(6,  78,  shortfp,             2, WHITE);
    int msg_x = (W - (int)strlen(message) * 12) / 2;
    if (msg_x < 0)
        msg_x = 0;

    draw_text(msg_x, 116, message, 2, WHITE);

    draw_text(30, 168, "GREEN APPROVE", 2, GREEN);
    draw_text(48, 198, "RED REJECT",    2, RED);

    lcd_flush();
}

static int show_certification_prompt(
    const char *uid,
    const uint8_t fingerprint[OPENPGP_V4_FINGERPRINT_LEN])
{
    const uint16_t BLACK = 0x0000;
    const uint16_t WHITE = 0xFFFF;
    const uint16_t GREEN = 0x07E0;
    const uint16_t RED   = 0xF800;

    static const char hex[] = "0123456789ABCDEF";

    char fp_hex[OPENPGP_V4_FINGERPRINT_LEN * 2 + 1];
    char fp_line1[21];
    char fp_line2[21];

    size_t uid_len;

    if (!uid || !fingerprint)
        return -1;

    uid_len = strlen(uid);

    if (uid_len == 0 || uid_len > TRUSTED_CERT_MAX_UID)
        return -1;

    if (!text_is_renderable(uid))
        return -1;

    for (size_t i = 0; i < OPENPGP_V4_FINGERPRINT_LEN; i++) {
        fp_hex[i * 2]     = hex[fingerprint[i] >> 4];
        fp_hex[i * 2 + 1] = hex[fingerprint[i] & 0x0F];
    }

    fp_hex[40] = '\0';

    memcpy(fp_line1, fp_hex, 20);
    fp_line1[20] = '\0';

    memcpy(fp_line2, fp_hex + 20, 20);
    fp_line2[20] = '\0';

    /*
     * Trusted certification review.
     *
     * Nothing is truncated:
     *   - UID must fit one scale-1 line.
     *   - all 40 fingerprint hex digits are displayed.
     */
    clear_screen(BLACK);

    draw_text(48, 10, "OPENPGP CERT", 2, WHITE);

    draw_text(90, 43, "TARGET UID", 1, WHITE);

    {
        int uid_x = (W - (int)uid_len * 6) / 2;

        if (uid_x < 0)
            return -1;

        draw_text(uid_x, 58, uid, 1, WHITE);
    }

    draw_text(66, 86, "KEY FINGERPRINT", 1, WHITE);

    draw_text(60, 103, fp_line1, 1, WHITE);
    draw_text(60, 116, fp_line2, 1, WHITE);

    draw_text(48, 162, "APPROVE CERT", 2, GREEN);
    draw_text(42, 198, "REJECT CANCEL", 2, RED);

    lcd_flush();

    return 0;
}

static int show_certification_review(
    const uint8_t *primary_key_body,
    size_t primary_key_body_len,
    const char *uid,
    uint8_t fingerprint_out[OPENPGP_V4_FINGERPRINT_LEN])
{
    uint8_t fingerprint[OPENPGP_V4_FINGERPRINT_LEN];

    if (!primary_key_body || !uid)
        return -1;

    /*
     * Security boundary:
     *
     * The target fingerprint is never accepted as authoritative input.
     * It is derived here from the exact primary-key body being reviewed.
     */
    if (openpgp_v4_primary_key_fingerprint(
            primary_key_body,
            primary_key_body_len,
            fingerprint) != 0)
        return -1;

    if (show_certification_prompt(uid, fingerprint) != 0)
        return -1;

    if (fingerprint_out)
        memcpy(fingerprint_out,
               fingerprint,
               OPENPGP_V4_FINGERPRINT_LEN);

    return 0;
}

static void show_result(int approved)
{
    const uint16_t WHITE = 0xFFFF;
    const uint16_t GREEN = 0x07E0;
    const uint16_t RED   = 0xF800;

    clear_screen(approved ? GREEN : RED);

    if (approved)
        draw_text(42, 105, "APPROVED", 3, WHITE);
    else
        draw_text(51, 105, "REJECTED", 3, WHITE);

    lcd_flush();
}


static void show_pin_entry(size_t digits)
{
    const uint16_t BLACK = 0x0000;
    const uint16_t WHITE = 0xFFFF;
    const uint16_t GREEN = 0x07E0;
    const uint16_t RED   = 0xF800;

    char mask[20];
    size_t shown = digits;

    if (shown > 16)
        shown = 16;

    memset(mask, 'X', shown);

    if (digits > 16)
        mask[shown++] = '+';

    mask[shown] = '\0';

    clear_screen(BLACK);

    draw_text(54, 35, "ENTER PIN", 3, WHITE);

    if (shown > 0) {
        int x = (W - (int)strlen(mask) * 12) / 2;
        if (x < 0)
            x = 0;

        draw_text(x, 105, mask, 2, WHITE);
    }

    draw_text(24, 170, "APPROVE SUBMIT", 2, GREEN);
    draw_text(30, 205, "REJECT CANCEL", 2, RED);

    lcd_flush();
}

static int wait_for_decision(void)
{
    int fd = open("/dev/input/event0", O_RDONLY);

    if (fd < 0) {
        perror("open /dev/input/event0");
        return -1;
    }

    struct input_event ev;

    while (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
        if (ev.type != EV_KEY || ev.value != 1)
            continue;

        if (ev.code == KEY_APPROVE) {
            close(fd);
            return 1;
        }

        if (ev.code == KEY_REJECT) {
            close(fd);
            return 0;
        }
    }

    close(fd);
    return -1;
}


static int show_certification_response_qr(
    const uint8_t *signature_packet,
    size_t signature_packet_len)
{
    const uint16_t WHITE = 0xFFFF;
    const uint16_t BLACK = 0x0000;

    static const char prefix[] =
        "KC1|OP=PGP_CERT_RESULT|CERT=";

    char packet_hex[513];
    char payload[640];

    if (!signature_packet ||
        signature_packet_len == 0 ||
        signature_packet_len * 2 + 1 > sizeof(packet_hex))
        return -1;

    for (size_t i = 0; i < signature_packet_len; i++)
        snprintf(&packet_hex[i * 2], 3, "%02X", signature_packet[i]);

    int n = snprintf(
        payload,
        sizeof(payload),
        "%s%s",
        prefix,
        packet_hex);

    if (n < 0 || (size_t)n >= sizeof(payload))
        return -1;

    QRcode *qr = QRcode_encodeString8bit(
        payload,
        0,
        QR_ECLEVEL_L);

    if (!qr) {
        fprintf(stderr, "Certification response QR encoding failed\n");
        return -1;
    }

    const int quiet = 4;
    const int modules = qr->width + quiet * 2;
    const int scale = W / modules;

    if (scale < 1) {
        fprintf(stderr,
                "Certification response QR too large for display\n");
        QRcode_free(qr);
        return -1;
    }

    const int rendered = modules * scale;
    const int x0 = (W - rendered) / 2;
    const int y0 = (H - rendered) / 2;

    clear_screen(WHITE);

    for (int y = 0; y < qr->width; y++) {
        for (int x = 0; x < qr->width; x++) {
            if (!(qr->data[y * qr->width + x] & 1))
                continue;

            int px = x0 + (x + quiet) * scale;
            int py = y0 + (y + quiet) * scale;

            for (int yy = 0; yy < scale; yy++)
                for (int xx = 0; xx < scale; xx++)
                    pixel(px + xx, py + yy, BLACK);
        }
    }

    lcd_flush();

    printf("Certification response QR version: %d\n", qr->version);
    printf("Certification response QR width: %d modules\n", qr->width);
    printf("Certification response QR scale: %d pixels/module\n", scale);
    printf("Certification response packet: %zu bytes\n",
           signature_packet_len);

    QRcode_free(qr);
    return 0;
}

static int show_response_qr(const char *fp,
                            const uint8_t *signature,
                            size_t signature_len)
{
    const uint16_t WHITE = 0xFFFF;
    const uint16_t BLACK = 0x0000;

    char sig_hex[257];
    char payload[512];

    if (signature_len * 2 + 1 > sizeof(sig_hex))
        return -1;

    for (size_t i = 0; i < signature_len; i++)
        snprintf(&sig_hex[i * 2], 3, "%02X", signature[i]);

    int n = snprintf(payload,
                     sizeof(payload),
                     "KC1|FP=%.*s|SIG=%s",
                     40,
                     fp,
                     sig_hex);

    if (n < 0 || (size_t)n >= sizeof(payload))
        return -1;

    QRcode *qr = QRcode_encodeString8bit(payload,
                                         0,
                                         QR_ECLEVEL_L);

    if (!qr) {
        fprintf(stderr, "QR encoding failed\n");
        return -1;
    }

    const int quiet = 4;
    const int modules = qr->width + quiet * 2;
    const int scale = W / modules;

    if (scale < 1) {
        fprintf(stderr, "QR too large for display\n");
        QRcode_free(qr);
        return -1;
    }

    const int rendered = modules * scale;
    const int x0 = (W - rendered) / 2;
    const int y0 = (H - rendered) / 2;

    clear_screen(WHITE);

    for (int y = 0; y < qr->width; y++) {
        for (int x = 0; x < qr->width; x++) {
            if (!(qr->data[y * qr->width + x] & 1))
                continue;

            int px = x0 + (x + quiet) * scale;
            int py = y0 + (y + quiet) * scale;

            for (int yy = 0; yy < scale; yy++)
                for (int xx = 0; xx < scale; xx++)
                    pixel(px + xx, py + yy, BLACK);
        }
    }

    lcd_flush();

    printf("QR version: %d\n", qr->version);
    printf("QR width: %d modules\n", qr->width);
    printf("QR scale: %d display pixels/module\n", scale);
    printf("QR payload:\n%s\n", payload);

    QRcode_free(qr);

    return 0;
}

int main(void)
{
    qr_sign_request_t request;

    char fp[40];
    uint8_t signature[128];
    size_t signature_len = sizeof(signature);

    uint32_t counter_before = 0;
    uint32_t counter_after = 0;

    lcd_init();

    clear_screen(0x0000);
    draw_text(48, 72, "SCAN REQUEST", 2, 0xFFFF);
    draw_text(18, 108, "SHOW QR TO CAMERA", 2, 0xFFFF);
    lcd_flush();

    if (qr_scan_sign_request("/dev/video0", &request) != 0) {
        clear_screen(0x0000);
        draw_text(30, 105, "SCAN ERROR", 3, 0xF800);
        lcd_flush();
        fprintf(stderr, "QR request scan failed\n");
        return 1;
    }

    size_t message_len = strlen(request.message);

    if (message_len > TRUSTED_DISPLAY_MAX_MESSAGE) {
        clear_screen(0x0000);
        draw_text(12, 105, "MSG TOO LONG", 3, 0xF800);
        lcd_flush();

        fprintf(stderr,
                "Rejected request: message is %zu characters; trusted display limit is %d\n",
                message_len,
                TRUSTED_DISPLAY_MAX_MESSAGE);

        return 1;
    }

    if (!text_is_renderable(request.message)) {
        clear_screen(0x0000);
        draw_text(48, 105, "BAD CHAR", 3, 0xF800);
        lcd_flush();

        fprintf(stderr,
                "Rejected request: message contains a character "
                "the trusted display cannot render\n");
        return 1;
    }

    const char *message = request.message;

    printf("Valid QR signing request received\n");
    printf("Exact scanned message: %s\n", message);

    if (neopgp_fingerprint(fp) != 0) {
        clear_screen(0x0000);
        draw_text(42, 100, "CARD ERROR", 3, 0xF800);
        lcd_flush();
        fprintf(stderr, "NeoPGP card detection failed\n");
        return 1;
    }

    printf("NeoPGP signing fingerprint: %.*s\n", 40, fp);

    if (neopgp_signature_counter(&counter_before) == 0)
        printf("Signature counter before: %u\n", counter_before);

    show_prompt(fp, message);

    int decision = wait_for_decision();

    if (decision < 0)
        return 1;

    if (decision == 0) {
        show_result(0);

        if (neopgp_signature_counter(&counter_after) == 0)
            printf("Signature counter after:  %u\n", counter_after);

        printf("REJECTED - no signing APDU executed\n");
        return 2;
    }

    show_result(1);

    printf("APPROVED\n");
    printf("Exact message: %s\n", message);
    printf("Requesting NeoPGP hardware signature...\n");

    int sign_result = neopgp_sign_message(message,
                                    signature,
                                    &signature_len,
                                    show_pin_entry);

    if (sign_result == 1) {
        clear_screen(0x0000);
        draw_text(18, 105, "PIN CANCELLED", 2, 0xF800);
        lcd_flush();

        if (neopgp_signature_counter(&counter_after) == 0)
            printf("Signature counter after:  %u\n", counter_after);

        printf("PIN ENTRY CANCELLED\n");
        return 2;
    }

    if (sign_result != 0) {

        clear_screen(0x0000);
        draw_text(27, 105, "SIGN ERROR", 3, 0xF800);
        lcd_flush();

        fprintf(stderr, "Hardware signing failed\n");
        return 1;
    }

    if (show_response_qr(fp, signature, signature_len) != 0) {
        clear_screen(0x0000);
        draw_text(42, 105, "QR ERROR", 3, 0xF800);
        lcd_flush();
        fprintf(stderr, "Could not render response QR\n");
        return 1;
    }

    printf("Hardware signature (%zu bytes):\n", signature_len);

    for (size_t i = 0; i < signature_len; i++)
        printf("%02X", signature[i]);

    printf("\n");

    if (neopgp_signature_counter(&counter_after) == 0)
        printf("Signature counter after:  %u\n", counter_after);

    return 0;
}
