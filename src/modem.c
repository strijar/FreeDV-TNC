/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  FreeDV TNC
 *
 *  Copyright (c) 2025 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <codec2/modem_stats.h>

#include "modem.h"
#include "util.h"
#include "audio.h"
#include "ptt.h"
#include "hdlc.h"
#include "tcp.h"

typedef enum {
    FRAME_SINGLE = 0,
    FRAME_BEGIN,
    FRAME_FRAG,
    FRAME_END
} fragment_t;

#define HEADER_SINGLE       0b00000000
#define HEADER_RESRV_1      0b00100000
#define HEADER_BEGIN        0b01000000
#define HEADER_FRAG         0b10000000
#define HEADER_END          0b11000000
#define HEADER_RESRV_2      0b11100000

#define HEADER_SINGLE_MASK  0b11100000
#define HEADER_BEGIN_MASK   0b11000000
#define HEADER_FRAG_MASK    0b11000000
#define HEADER_END_MASK     0b11100000

static struct freedv    *freedv = NULL;
static int              frame_bytes;
static uint8_t          *frame_tx = NULL;
static int              payload_bytes;
static int16_t          *samples_tx = NULL;
static int              samples_num;
static int              samples_max;

static uint32_t         tx_timeout = 0;
static bool             tx_enable = false;

static float            db_avr = 0.0f;
static size_t           samples_rx_index = 0;
static int16_t          *samples_rx = NULL;
static uint8_t          *bytes_rx = NULL;
static fragment_t       prev_frag_rx = FRAME_SINGLE;
static uint8_t          prev_id_rx = 0;
static uint8_t          frame_rx[MTU];
static size_t           frame_rx_index = 0;

void modem_init() {
    struct freedv_advanced  adv = {0, 4, 500, 8000, 1000, 500, "H_256_768_22"};
    int                     mode = FREEDV_MODE_FSK_LDPC;

    freedv = freedv_open_advanced(mode, &adv);

    freedv_set_verbose(freedv, 0);
    freedv_set_frames_per_burst(freedv, 1);

    frame_bytes = freedv_get_bits_per_modem_frame(freedv) / 8;
    payload_bytes = frame_bytes - 1 - 2; /* Header and CRC */
    samples_num = freedv_get_n_tx_modem_samples(freedv);
    samples_max = freedv_get_n_max_modem_samples(freedv);

    printf("Payload %d bytes -> %d samples\n", payload_bytes, samples_num);

    frame_tx = (uint8_t *) malloc(frame_bytes);
    samples_tx = (int16_t *) malloc(samples_num * sizeof(int16_t));
    samples_rx = (int16_t *) malloc(samples_max * sizeof(int16_t) * 2);
    bytes_rx = (uint8_t *) malloc(frame_bytes);

    if (mode == FREEDV_MODE_FSK_LDPC) {
        printf("Frequency: Fs: %4.1f Hz Rs: %5.0f Hz Tone1: %5.0f Hz Shift: "
              "%5.0f Hz M: %d \n",
              (float)adv.Fs, (float)adv.Rs, (float)adv.first_tone,
              (float)adv.tone_spacing, adv.M);

        if (adv.tone_spacing < adv.Rs) {
            printf("Need shift: %d > Rs: %d\n", adv.tone_spacing, adv.Rs);
            exit(1);
        }
    }
}

static void send_preamble() {
    int len = freedv_rawdatapreambletx(freedv, samples_tx);

    if (len > 0) {
        audio_send(samples_tx, len);
    }
}

static void send_data() {
    freedv_rawdatatx(freedv, samples_tx, frame_tx);

    audio_send(samples_tx, samples_num);
}

static void send_postamble() {
    int len = freedv_rawdatapostambletx(freedv, samples_tx);

    if (len > 0) {
        audio_send(samples_tx, len);
    }
}

static void send_silence(int len) {
    uint16_t buf[len * 2];

    memset(buf, 0, len * 2);
    audio_send(buf, len);
}

static void encode_frame(const uint8_t *buf, size_t len, fragment_t type, uint8_t id) {
    uint8_t     header = 0;
    uint16_t    crc;

    switch (type) {
        case FRAME_SINGLE:
            header = HEADER_SINGLE | (len & ~HEADER_SINGLE_MASK);
            break;

        case FRAME_BEGIN:
            header = HEADER_BEGIN | (id & ~HEADER_BEGIN_MASK);
            break;

        case FRAME_FRAG:
            header = HEADER_FRAG | (id & ~HEADER_FRAG_MASK);
            break;

        case FRAME_END:
            header = HEADER_END | (len & ~HEADER_END_MASK);
            break;
    }

    memset(frame_tx, 0, frame_bytes);

    frame_tx[0] = header;
    memcpy(&frame_tx[1], buf, len);

    crc = freedv_gen_crc16(frame_tx, payload_bytes + 1);
    frame_tx[frame_bytes - 2] = crc >> 8;
    frame_tx[frame_bytes - 1] = crc & 0xFF;
}

void decode_frame(uint8_t *buf, int len) {
    uint8_t header = buf[0];
    uint8_t append = 0;
    bool    send = false;
    bool    reset = false;
    uint8_t id;

    buf++;      /* Skip header */
    len -= 3;   /* Header, CRC */

    if ((header & HEADER_SINGLE_MASK) == HEADER_SINGLE) {
        append = header & ~HEADER_SINGLE_MASK;
        send = true;
    } else if ((header & HEADER_BEGIN_MASK) == HEADER_BEGIN) {
        append = len;
        prev_frag_rx = FRAME_BEGIN;
        prev_id_rx = header & ~HEADER_BEGIN_MASK;
    } else if ((header & HEADER_FRAG_MASK) == HEADER_FRAG) {
        if (prev_frag_rx == FRAME_BEGIN || prev_frag_rx == FRAME_FRAG) {
            id = header & ~HEADER_FRAG_MASK;

            if (prev_id_rx == id + 1) {
                append = len;
                prev_frag_rx = FRAME_FRAG;
                prev_id_rx = id;
            } else {
                printf("Lost frag\n");
                reset = true;
            }
        } else {
            printf("Lost begin\n");
            reset = true;
        }
    } else if ((header & HEADER_END_MASK) == HEADER_END) {
        if (prev_frag_rx == FRAME_BEGIN || prev_frag_rx == FRAME_FRAG) {
            if (prev_id_rx == 0) {
                append = header & ~HEADER_END_MASK;
                send = true;
            } else {
                printf("Lost frag\n");
                reset = true;
            }
        } else {
            printf("Lost begin\n");
            reset = true;
        }
    }

    if (append) {
        memcpy(&frame_rx[frame_rx_index], buf, append);
        frame_rx_index += append;
    }

    if (send) {
        hdlc_encode(frame_rx, frame_rx_index);
        reset = true;
    }

    if (reset) {
        prev_frag_rx = FRAME_SINGLE;
        frame_rx_index = 0;
    }
}

void modem_send(const uint8_t *buf, size_t len) {
    int         left = len;
    uint8_t     part;

    if (freedv == NULL || frame_tx == NULL) {
        return;
    }

    while (!tx_enable) {
        usleep(100000);
    }

    ptt_set(true);
    send_silence(4000);
    send_preamble();

    if (len <= payload_bytes) {
        encode_frame(buf, len, FRAME_SINGLE, 0);
        send_data();
    } else {
        fragment_t  frag = FRAME_BEGIN;
        uint8_t     id = len / payload_bytes - 1;

        while (left > 0) {
            part = left < payload_bytes ? left : payload_bytes;

            encode_frame(buf, part, frag, id);
            send_data();

            left -= part;
            buf += part;

            if (id == 0) {
                frag = FRAME_END;
            } else {
                frag = FRAME_FRAG;
                id--;
            }
        }
    }

    send_postamble();
    send_silence(2000);
    audio_wait();
    ptt_set(false);
}

void modem_recv(const int16_t *buf, size_t len) {
    if (ptt_is_on() || freedv == NULL || samples_rx == NULL) {
        return;
    }

    float db = signal_db(buf, len);

    if (db_avr == 0.0f) {
        db_avr = db;
    } else {
        db_avr = db_avr * 0.75f + db * 0.25f;
    }

    if (tx_enable) {
        if (db_avr > -55.0f) {
            tx_enable = false;
            tx_timeout = 0;
            printf("Air busy\n");
        }
    } else {
        if (db_avr < -60.0f) {
            if (tx_timeout > 1000) {
                tx_enable = true;
                printf("Air free\n");
            } else {
                tx_timeout += len;
            }
        }
    }

    memcpy(&samples_rx[samples_rx_index], buf, len * sizeof(int16_t));
    samples_rx_index += len;

    int nin = freedv_nin(freedv);

    while (samples_rx_index >= nin) {
        int nbytes = freedv_rawdatarx(freedv, bytes_rx, samples_rx);

        samples_rx_index -= nin;

        if (samples_rx_index > 0) {
            memmove(samples_rx, &samples_rx[nin], samples_rx_index * sizeof(int16_t));
        }

        if (nbytes) {
            struct MODEM_STATS stats;

            freedv_get_modem_extended_stats(freedv, &stats);
            decode_frame(bytes_rx, nbytes);
        }

        nin = freedv_nin(freedv);
    }
}
