/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  FreeDV TNC
 *
 *  Copyright (c) 2025 Belousov Oleg aka R1CBU
 */

#include <stdbool.h>

#include "hdlc.h"
#include "util.h"
#include "modem.h"

#define FLAG        0x7E
#define ESC         0x7D
#define ESC_MASK    0x20

static uint8_t  buf_in[1024];
static size_t   len_in = 0;
static bool     in_frame = false;
static bool     escape = false;

void hdlc_decode(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = buf[i];

        if (in_frame && byte == FLAG) {
            modem_send(buf_in, len_in);

            in_frame = false;
            len_in = 0;
        } else if (byte == FLAG) {
            in_frame = true;
            len_in = 0;
        } else if (in_frame) {
            if (byte == ESC) {
                escape = true;
            } else {
                if (escape) {
                    if (byte == FLAG ^ ESC_MASK) {
                        byte = FLAG;
                    }
                    if (byte == ESC ^ ESC_MASK) {
                        byte = ESC;
                    }
                    escape = false;
                }
                buf_in[len_in] = byte;
                len_in++;

                if (len_in >= sizeof(buf_in)) {
                    len_in = 0;
                    escape = false;
                    in_frame = false;

                    return;
                }
            }
        }
    }
}
