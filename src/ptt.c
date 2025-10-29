/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  FreeDV TNC
 *
 *  Copyright (c) 2025 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>
#include <stdio.h>
#include <gpiod.h>

#include "ptt.h"

static struct gpiod_line    *line_ptt = NULL;
static bool                 state = false;

void ptt_init() {
    struct gpiod_chip    *chip = NULL;

    chip = gpiod_chip_open_by_number(0);

    if (chip) {
        line_ptt = gpiod_chip_get_line(chip, 7);

        if (line_ptt) {
            gpiod_line_request_output(line_ptt, "PTT", 0);
        } else {
            printf("Unable to open gpio pin 7");
        }
    } else {
        printf("Unable to open gpio chip 0");
    }
}

void ptt_set(bool on) {
    state = on;
    printf("PTT: %s\n", on ? "On" : "Off");
    gpiod_line_set_value(line_ptt, on ? 1 : 0);
}

bool ptt_is_on() {
    return state;
}
