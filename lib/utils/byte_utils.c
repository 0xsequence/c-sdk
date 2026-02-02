#include "byte_utils.h"

#include <stdlib.h>
#include <string.h>

uint8_t *prepend_zero(const uint8_t *in, size_t in_len, size_t *out_len) {
    uint8_t *out = malloc(in_len + 1);
    if (!out) return NULL;

    out[0] = 0x00;
    memcpy(out + 1, in, in_len);

    if (out_len) *out_len = in_len + 1;
    return out;
}

size_t string_to_bytes(const char *str, uint8_t *out, size_t out_len)
{
    size_t i = 0;

    while (str[i] && i < out_len) {
        out[i] = (uint8_t)str[i];
        i++;
    }

    return i;
}
