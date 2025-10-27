/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  FreeDV TNC
 *
 *  Copyright (c) 2025 Belousov Oleg aka R1CBU
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "tcp.h"
#include "modem.h"
#include "audio.h"

int main() {
    audio_init();
    tcp_init(8080);
    modem_open(FREEDV_MODE_DATAC0, NULL);

    while (true) {
        tcp_read();
    }

    return 0;
}
