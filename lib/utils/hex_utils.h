#include <stddef.h>
#include <stdint.h>

uint8_t *hex_to_bytes(const char *hex, size_t *out_len);
char *bytes_to_hex(const uint8_t *bytes, size_t len);
