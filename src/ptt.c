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
#include <syslog.h>

#include "ptt.h"
#include "config.h"

static struct gpiod_line    *line_ptt = NULL;
static bool                 state = false;

void ptt_init() {
    struct gpiod_chip    *chip = NULL;

    chip = gpiod_chip_open_by_number(config->ptt.port);

    if (chip) {
        line_ptt = gpiod_chip_get_line(chip, config->ptt.pin);

        if (line_ptt) {
            gpiod_line_request_output(line_ptt, "PTT", 0);
        } else {
            syslog(LOG_ERR, "Unable to open gpio pin %i", config->ptt.pin);
        }
    } else {
        syslog(LOG_ERR, "Unable to open gpio chip %i", config->ptt.port);
    }
}

void ptt_set(bool on) {
    state = on;
    syslog(LOG_INFO, "PTT: %s\n", on ? "On" : "Off");
    gpiod_line_set_value(line_ptt, on ? 1 : 0);
}

bool ptt_is_on() {
    return state;
}
