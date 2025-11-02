#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "util.h"

void dump(char *label, const uint8_t *buf, size_t len) {
    printf("%s %i: ", label, len);

    for (size_t i = 0; i < len; i++)
        printf("%02X ", buf[i]);

    printf("\n");
}

float signal_db(const int16_t *buf, size_t len) {
    float sum = 0;

    for (int i = 0; i < len; i++) {
        float x = buf[i] / 32768.0f;

        sum = x * x;
    }

    return sum > 0 ? 10.0f * log10f(sum) : -60.0f;
}
