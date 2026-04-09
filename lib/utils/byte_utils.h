#ifndef SEQUENCE_BYTE_UTILS_H
#define SEQUENCE_BYTE_UTILS_H

#include <stddef.h>
#include <stdint.h>

uint8_t *prepend_zero(const uint8_t *in, size_t in_len, size_t *out_len);
size_t string_to_bytes(const char *str, uint8_t *out, size_t out_len);

#endif
