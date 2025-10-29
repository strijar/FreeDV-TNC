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

static struct freedv    *freedv = NULL;
static int              frame_bytes;
static uint8_t          *frame_tx = NULL;
static int              payload_bytes;
static int16_t          *samples_tx = NULL;
static int              samples_num;
static int              samples_max;

static size_t           samples_rx_index = 0;
static int16_t          *samples_rx = NULL;
static uint8_t          *bytes_rx = NULL;

void modem_init() {
    struct freedv_advanced  adv = {0, 4, 400, 8000, 1000, 400, "H_256_512_4"};
    int                     mode = FREEDV_MODE_FSK_LDPC;

    freedv = freedv_open_advanced(mode, &adv);

    freedv_set_verbose(freedv, 1);
    freedv_set_frames_per_burst(freedv, 1);

    frame_bytes = freedv_get_bits_per_modem_frame(freedv) / 8;
    payload_bytes = frame_bytes - 2; /* 16 bits used for the CRC */
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
    memset(samples_tx, 0, len * 2);
    audio_send(samples_tx, len);
}

void modem_send(const uint8_t *buf, size_t len) {
    int         left = len;
    int         part;
    uint16_t    crc;

    if (freedv == NULL || frame_tx == NULL) {
        return;
    }

    dump("Modem", buf, len);

    ptt_set(true);
    send_silence(4000);
    send_preamble();

    while (left > 0) {
        part = left < payload_bytes ? left : payload_bytes;

        memset(frame_tx, 0, frame_bytes);
        memcpy(frame_tx, buf, part);

        crc = freedv_gen_crc16(frame_tx, payload_bytes);
        frame_tx[frame_bytes - 2] = crc >> 8;
        frame_tx[frame_bytes - 1] = crc & 0xFF;

        send_data();

        left -= part;
        buf += part;
    }

    send_postamble();
    send_silence(4000);
    audio_wait();
    ptt_set(false);
}

void modem_recv(const int16_t *buf, size_t len) {
    if (ptt_is_on() || freedv == NULL || samples_rx == NULL) {
        return;
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

            nbytes -= 2;
            dump("Recv", bytes_rx, nbytes);
        }

        nin = freedv_nin(freedv);
    }
}
