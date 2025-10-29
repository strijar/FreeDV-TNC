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

void modem_init();

void modem_send(const uint8_t *buf, size_t len);
void modem_recv(const int16_t *buf, size_t len);
