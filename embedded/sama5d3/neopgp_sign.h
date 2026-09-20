#ifndef NEOPGP_SIGN_H
#define NEOPGP_SIGN_H

#include <stddef.h>
#include <stdint.h>

typedef void (*neopgp_pin_progress_fn)(size_t digits);

int neopgp_sign_message(const char *message,
                        uint8_t *signature,
                        size_t *signature_len,
                        neopgp_pin_progress_fn pin_progress);

int neopgp_signature_counter(uint32_t *counter);

#endif
