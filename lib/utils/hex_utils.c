#include "hex_utils.h"
#include <stdlib.h>
#include <string.h>

uint8_t *hex_to_bytes(const char *hex, size_t *out_len) {
    if (!hex) return NULL;

    // Optional 0x prefix
    if (hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
        hex += 2;
    }

    size_t hex_len = strlen(hex);
    if (hex_len % 2 != 0) return NULL;

    size_t len = hex_len / 2;
    uint8_t *out = malloc(len);
    if (!out) return NULL;

    for (size_t i = 0; i < len; i++) {
        char c1 = hex[i * 2];
        char c2 = hex[i * 2 + 1];

        uint8_t n1 =
            (c1 >= '0' && c1 <= '9') ? c1 - '0' :
            (c1 >= 'a' && c1 <= 'f') ? c1 - 'a' + 10 :
            (c1 >= 'A' && c1 <= 'F') ? c1 - 'A' + 10 : 255;

        uint8_t n2 =
            (c2 >= '0' && c2 <= '9') ? c2 - '0' :
            (c2 >= 'a' && c2 <= 'f') ? c2 - 'a' + 10 :
            (c2 >= 'A' && c2 <= 'F') ? c2 - 'A' + 10 : 255;

        if (n1 == 255 || n2 == 255) {
            free(out);
            return NULL;
        }

        out[i] = (n1 << 4) | n2;
    }

    if (out_len) *out_len = len;
    return out;
}

char *bytes_to_hex(const uint8_t *bytes, size_t len) {
    static const char hex[] = "0123456789abcdef";

    size_t out_len = 2 + len * 2 + 1;
    char *out = malloc(out_len);
    if (!out) return NULL;

    out[0] = '0';
    out[1] = 'x';

    for (size_t i = 0; i < len; i++) {
        out[2 + i * 2]     = hex[bytes[i] >> 4];
        out[2 + i * 2 + 1] = hex[bytes[i] & 0x0F];
    }

    out[2 + len * 2] = '\0';
    return out;
}