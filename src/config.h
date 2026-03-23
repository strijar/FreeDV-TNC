/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  FreeDV TNC
 *
 *  Copyright (c) 2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t     port;
    uint8_t     pin;
} config_gpio_t;

typedef struct {
    uint8_t     fsk;
    uint16_t    rate;
    uint16_t    first_tone;
    uint16_t    tone_spacing;
    char        *codename;
} config_modem_t;

typedef struct {
    uint16_t    sig_freq;
    uint16_t    modem_bytes;
    uint32_t    modem_wait;
    uint16_t    sinad_start;
    uint16_t    sinad_stop;
} config_send_t;

typedef struct {
    config_gpio_t   ptt;
    uint32_t        tcp_port;
    config_modem_t  modem;
    config_send_t   send;
} config_t;

bool config_load(const char *filename);

extern config_t *config;
