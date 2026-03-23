/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  FreeDV TNC
 *
 *  Copyright (c) 2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

void sinad_init();
void sinad_calc(const int16_t *buf, size_t len);
