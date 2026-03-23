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
#include <signal.h>
#include <syslog.h>
#include <math.h>

#include "tcp.h"
#include "modem.h"
#include "audio.h"
#include "ptt.h"
#include "config.h"
#include "sinad.h"

void signal_handler(int signum) {
    if (ptt_is_on()) {
        ptt_set(false);
    }

    exit(0);
}

static void using() {
    printf("FreeDV-TNC modem\nUsing: freedv-tnc <config_file.yaml> (work | send-sig | recv-sig | send-modem | recv-modem)\n");
}

static void work() {
    modem_init(MODEM_WORK);
    tcp_init(8080);

    while (true) {
        tcp_read();
    }
}

static void send_sig() {
    int16_t samples = 8000;

    float   delta = 2.0f * M_PI * config->send.sig_freq / 8000.0f;
    float   phase = 0.0;
    int16_t buf[samples];

    usleep(250000);

    printf("Send test signal - %iHz. Press <CTRL-C> to stop\n", config->send.sig_freq);
    ptt_set(true);

    while (true) {
        for (int16_t i = 0; i < samples; i++) {
            buf[i] = sinf(phase) * 32767.0f;
            phase += delta;
        }

        audio_send(buf, samples);
    }
}

static void recv_sig() {
    sinad_init();

    modem_init(MODEM_RECV_SIG);
    printf("Receiving test signal. Press <CTRL-C> to stop\n");

    while (true) {
        usleep(100000);
    }
}

static void send_modem() {
    uint8_t     size = config->send.modem_bytes;
    uint32_t    wait = config->send.modem_wait;
    uint8_t     data[size];

    modem_init(MODEM_SEND);

    for (uint8_t i = 0; i < size; i++)
        data[i] = i;

    while (true) {
        printf("Send test %i bytes. Press <CTRL-C> to stop\n", size);
        modem_send(data, size);

        printf("Wait %ims...\n", wait);
        usleep(wait * 1000);
    }
}

static void recv_modem() {
    modem_init(MODEM_RECV);
    printf("Receiving test bytes. Press <CTRL-C> to stop\n");

    while (true) {
        usleep(100000);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        using();
        return 1;
    }

    openlog("freedv-tnc", LOG_PID, LOG_USER);

    if (!config_load(argv[1])) {
        return 1;
    }

    struct sigaction sa;

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    ptt_init();
    audio_init();

    const char *mode = argv[2];

    if (strcmp(mode, "work") == 0) {
        work();
    } else if (strcmp(mode, "send-sig") == 0) {
        send_sig();
    } else if (strcmp(mode, "recv-sig") == 0) {
        recv_sig();
    } else if (strcmp(mode, "send-modem") == 0) {
        send_modem();
    } else if (strcmp(mode, "recv-modem") == 0) {
        recv_modem();
    } else {
        using();
        return 1;
    }

    return 0;
}
