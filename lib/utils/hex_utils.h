#ifndef OMS_WALLET_HEX_UTILS_H
#define OMS_WALLET_HEX_UTILS_H

#include <stddef.h>
#include <stdint.h>

uint8_t *hex_to_bytes(const char *hex, size_t *out_len);
char *bytes_to_hex(const uint8_t *bytes, size_t len);

#endif
