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

#include <codec2/freedv_api.h>

#define MTU 1024

typedef enum {
    MODEM_WORK = 0,
    MODEM_SEND,
    MODEM_RECV,
    MODEM_RECV_SIG
} modem_mode_t;

void modem_init(modem_mode_t mode);

void modem_send(const uint8_t *buf, size_t len);
void modem_recv(const int16_t *buf, size_t len);
