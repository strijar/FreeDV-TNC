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
#include "ptt.h"

int main() {
    ptt_init();
    audio_init();
    modem_init();
    tcp_init(8080);

    while (true) {
        tcp_read();
    }

    return 0;
}
