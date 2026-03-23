/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  FreeDV TNC
 *
 *  Copyright (c) 2025 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>
#include <fftw3.h>
#include <math.h>

#include "sinad.h"
#include "config.h"

#define FFT 1024
#define FS  8000.0

static fftw_plan    p;
static double       *in;
static uint16_t     in_index;
static fftw_complex *out;
static double       *window;

static uint16_t     start;
static uint16_t     stop;

void sinad_init() {
    in_index = 0;
    in = (double*) fftw_malloc(sizeof(double) * FFT);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (FFT/2 + 1));
    p = fftw_plan_dft_r2c_1d(FFT, in, out, FFTW_ESTIMATE);

    window = (double *) malloc(sizeof(double) * FFT);

    for (int i = 0; i < FFT; i++) {
        window[i] = 0.5 * (1.0 - cos(2.0 * M_PI * i / (FFT - 1)));
    }

    start = config->send.sinad_start * FFT / FS;
    stop = config->send.sinad_stop * FFT / FS;
}

static void find_peak() {
    double      max = 0.0;
    double      total = 0.0;
    uint16_t    index = 0;

    for (uint16_t i = start; i < stop; i++) {
        double power = out[i][0] * out[i][0] + out[i][1] * out[i][1];

        total += power;

        if (power > max) {
            max = power;
            index = i;
        }
    }

    double signal = 0;
    double sinad = 0;

    for (uint16_t i = index - 2; i <= index + 2; i++) {
        double power = out[i][0] * out[i][0] + out[i][1] * out[i][1];

        signal += power;
    }

    if (total > signal && total > 0) {
        sinad = 10.0 * log10(total / (total - signal));
    }

    double y1 = sqrt(out[index - 1][0] * out[index - 1][0] + out[index - 1][1] * out[index - 1][1]);
    double y2 = sqrt(out[index][0] * out[index][0] + out[index][1] * out[index][1]);
    double y3 = sqrt(out[index + 1][0] * out[index + 1][0] + out[index + 1][1] * out[index + 1][1]);

    double delta = 0.5 * (y1 - y3) / (y1 - 2.0 * y2 + y3);
    double freq = (index + delta) * (FS / FFT);

    signal = 10.0 * log10(signal);

    printf("Peak: %.1f Hz, power: %.1f dB, sinad: %.1f dB\n", freq, signal, sinad);
}

void sinad_calc(const int16_t *buf, size_t len) {
    for (uint16_t i = 0; i < len; i++) {
        in[in_index] = buf[i] / 32768.0;

        in_index++;

        if (in_index >= FFT) {
            for (uint16_t w = 0; w < FFT; w++) {
                in[w] *= window[w];
            }

            fftw_execute(p);
            find_peak();

            in_index = 0;
        }
    }
}
