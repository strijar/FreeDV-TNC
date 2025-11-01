#include <stdint.h>
#include <stddef.h>

void dump(char *label, const uint8_t *buf, size_t len);
float signal_db(const int16_t *buf, size_t len);
