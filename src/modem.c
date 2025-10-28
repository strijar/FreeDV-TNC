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

#include "modem.h"
#include "util.h"
#include "audio.h"
#include "ptt.h"

static struct freedv    *freedv;
static int              frame_bytes;
static uint8_t          *frame_tx;
static int              payload_bytes;
static int16_t          *samples_tx;
static int              samples_num;

void modem_open(int mode, struct freedv_advanced *adv) {
    if (mode != FREEDV_MODE_FSK_LDPC) {
        freedv = freedv_open(mode);
    } else {
        freedv = freedv_open_advanced(mode, adv);
    }

    frame_bytes = freedv_get_bits_per_modem_frame(freedv) / 8;
    payload_bytes = frame_bytes - 2; /* 16 bits used for the CRC */
    samples_num = freedv_get_n_tx_modem_samples(freedv);

    printf("Payload %d bytes -> %d samples\n", payload_bytes, samples_num);

    frame_tx = (uint8_t *) malloc(frame_bytes);
    samples_tx = (int16_t *) malloc(samples_num * 2);

    if (mode == FREEDV_MODE_FSK_LDPC) {
        printf("Frequency: Fs: %4.1f Hz Rs: %5.0f Hz Tone1: %5.0f Hz Shift: "
              "%5.0f Hz M: %d \n",
              (float)adv->Fs, (float)adv->Rs, (float)adv->first_tone,
              (float)adv->tone_spacing, adv->M);

        if (adv->tone_spacing < adv->Rs) {
            printf("Need shift: %d > Rs: %d\n", adv->tone_spacing, adv->Rs);
            exit(1);
        }
    }
}

static void send_preamble() {
    int len = freedv_rawdatapreambletx(freedv, samples_tx);

    audio_send(samples_tx, len);
}

static void send_data() {
    freedv_rawdatatx(freedv, samples_tx, frame_tx);

    audio_send(samples_tx, samples_num);
}

static void send_postamble() {
    int len = freedv_rawdatapostambletx(freedv, samples_tx);

    audio_send(samples_tx, len);
}

void modem_send(const uint8_t *buf, size_t len) {
    int         left = len;
    int         part;
    uint16_t    crc;

    dump("Modem", buf, len);
    ptt_set(true);

    while (left > 0) {
        part = left < payload_bytes ? left : payload_bytes;

        memset(frame_tx, 0, frame_bytes);
        memcpy(frame_tx, buf, part);

        crc = freedv_gen_crc16(frame_tx, payload_bytes);
        frame_tx[frame_bytes - 2] = crc >> 8;
        frame_tx[frame_bytes - 1] = crc & 0xFF;

        send_preamble();
        send_data();
        send_postamble();

        left -= part;
        buf += part;
    }

    audio_wait();
    ptt_set(false);
}
