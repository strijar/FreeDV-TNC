/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  FreeDV TNC
 *
 *  Copyright (c) 2025 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>
#include <stdio.h>

#include "ptt.h"

void ptt_init() {
}

void ptt_set(bool on) {
    printf("PTT: %s\n", on ? "On" : "Off");
}
