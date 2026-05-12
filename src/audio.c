/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  FreeDV TNC
 *
 *  Copyright (c) 2025 Belousov Oleg aka R1CBU
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <alsa/asoundlib.h>

#include "audio.h"
#include "modem.h"

#define SAMPLE_RATE     8000
#define CHANNELS        1
#define PLAY_RATE_MS    25
#define CAPTURE_RATE_MS 25
#define ALSA_DEVICE     "default"

/* Frames per period derived from rate and interval */
#define PLAY_PERIOD_FRAMES    ((SAMPLE_RATE * PLAY_RATE_MS)    / 1000)
#define CAPTURE_PERIOD_FRAMES ((SAMPLE_RATE * CAPTURE_RATE_MS) / 1000)

static snd_pcm_t    *play_pcm    = NULL;
static snd_pcm_t    *capture_pcm = NULL;

static pthread_t     capture_thread;
static volatile bool capture_running = false;

/* ------------------------------------------------------------------ */
/*  Helpers                                                             */
/* ------------------------------------------------------------------ */

static int set_hw_params(snd_pcm_t *pcm, snd_pcm_uframes_t period_frames,
                         unsigned int periods)
{
    snd_pcm_hw_params_t *hw;
    int err;

    snd_pcm_hw_params_alloca(&hw);

    if ((err = snd_pcm_hw_params_any(pcm, hw)) < 0) {
        fprintf(stderr, "audio: hw_params_any: %s\n", snd_strerror(err));
        return err;
    }

    if ((err = snd_pcm_hw_params_set_access(pcm, hw,
                    SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        fprintf(stderr, "audio: set_access: %s\n", snd_strerror(err));
        return err;
    }

    if ((err = snd_pcm_hw_params_set_format(pcm, hw,
                    SND_PCM_FORMAT_S16_LE)) < 0) {
        fprintf(stderr, "audio: set_format: %s\n", snd_strerror(err));
        return err;
    }

    if ((err = snd_pcm_hw_params_set_channels(pcm, hw, CHANNELS)) < 0) {
        fprintf(stderr, "audio: set_channels: %s\n", snd_strerror(err));
        return err;
    }

    unsigned int rate = SAMPLE_RATE;

    if ((err = snd_pcm_hw_params_set_rate_near(pcm, hw, &rate, 0)) < 0) {
        fprintf(stderr, "audio: set_rate: %s\n", snd_strerror(err));
        return err;
    }

    snd_pcm_uframes_t pf = period_frames;

    if ((err = snd_pcm_hw_params_set_period_size_near(pcm, hw, &pf, 0)) < 0) {
        fprintf(stderr, "audio: set_period_size: %s\n", snd_strerror(err));
        return err;
    }

    snd_pcm_uframes_t buf_frames = pf * periods;

    if ((err = snd_pcm_hw_params_set_buffer_size_near(pcm, hw,
                    &buf_frames)) < 0) {
        fprintf(stderr, "audio: set_buffer_size: %s\n", snd_strerror(err));
        return err;
    }

    if ((err = snd_pcm_hw_params(pcm, hw)) < 0) {
        fprintf(stderr, "audio: hw_params: %s\n", snd_strerror(err));
        return err;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Capture thread                                                      */
/* ------------------------------------------------------------------ */

static void *capture_thread_fn(void *arg)
{
    (void) arg;

    const snd_pcm_uframes_t frames = CAPTURE_PERIOD_FRAMES;
    int16_t buf[frames];

    while (capture_running) {
        snd_pcm_sframes_t n = snd_pcm_readi(capture_pcm, buf, frames);

        if (n == -EPIPE) {
            /* Overrun — recover and continue */
            snd_pcm_prepare(capture_pcm);
            continue;
        } else if (n < 0) {
            int err = snd_pcm_recover(capture_pcm, (int) n, 0);

            if (err < 0) {
                fprintf(stderr, "audio: capture recover: %s\n",
                        snd_strerror(err));
                break;
            }

            continue;
        }

        modem_recv(buf, (int) n);
    }

    return NULL;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

void audio_init()
{
    int err;

    /* --- Playback --- */
    err = snd_pcm_open(&play_pcm, ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);

    if (err < 0) {
        fprintf(stderr, "audio: open playback: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    if (set_hw_params(play_pcm, PLAY_PERIOD_FRAMES, 30) < 0) {
        exit(EXIT_FAILURE);
    }

    if ((err = snd_pcm_prepare(play_pcm)) < 0) {
        fprintf(stderr, "audio: prepare playback: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    /* --- Capture --- */
    err = snd_pcm_open(&capture_pcm, ALSA_DEVICE, SND_PCM_STREAM_CAPTURE, 0);

    if (err < 0) {
        fprintf(stderr, "audio: open capture: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    if (set_hw_params(capture_pcm, CAPTURE_PERIOD_FRAMES, 2) < 0) {
        exit(EXIT_FAILURE);
    }

    if ((err = snd_pcm_prepare(capture_pcm)) < 0) {
        fprintf(stderr, "audio: prepare capture: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    if ((err = snd_pcm_start(capture_pcm)) < 0) {
        fprintf(stderr, "audio: start capture: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    /* Spawn capture thread */
    capture_running = true;

    if (pthread_create(&capture_thread, NULL, capture_thread_fn, NULL) != 0) {
        perror("audio: pthread_create");
        exit(EXIT_FAILURE);
    }
}

void audio_send(const int16_t *buf, int len)
{
    if (play_pcm == NULL || buf == NULL || len == 0) {
        return;
    }

    const int16_t *ptr   = buf;
    int            remaining = len;   /* in frames (samples for mono) */

    while (remaining > 0) {
        snd_pcm_sframes_t written = snd_pcm_writei(play_pcm, ptr, remaining);

        if (written == -EPIPE) {
            /* Underrun — recover and retry */
            snd_pcm_prepare(play_pcm);
            continue;
        } else if (written < 0) {
            int err = snd_pcm_recover(play_pcm, (int) written, 0);

            if (err < 0) {
                fprintf(stderr, "audio: writei recover: %s\n",
                        snd_strerror(err));
                return;
            }

            continue;
        }

        ptr       += written;
        remaining -= (int) written;
    }
}

void audio_wait()
{
    if (play_pcm == NULL) {
        return;
    }

    snd_pcm_drain(play_pcm);
    snd_pcm_prepare(play_pcm);   /* ready for next audio_send() call */
}
