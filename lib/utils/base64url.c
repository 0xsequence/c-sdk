#include "base64url.h"

#include <stdlib.h>

static const char k_sequence_base64url_alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

size_t sequence_base64url_unpadded_encoded_len(size_t len)
{
    size_t full_groups = len / 3;
    size_t remainder = len % 3;
    size_t encoded_len = full_groups * 4;

    if (remainder == 1)
    {
        encoded_len += 2;
    }
    else if (remainder == 2)
    {
        encoded_len += 3;
    }

    return encoded_len;
}

char *sequence_base64url_encode_unpadded(const uint8_t *data, size_t len)
{
    size_t encoded_len;
    char *encoded;
    size_t in_index = 0;
    size_t out_index = 0;

    if (!data && len != 0)
    {
        return NULL;
    }

    encoded_len = sequence_base64url_unpadded_encoded_len(len);
    encoded = malloc(encoded_len + 1);
    if (!encoded)
    {
        return NULL;
    }

    while (in_index + 3 <= len)
    {
        uint32_t chunk = ((uint32_t)data[in_index] << 16) |
                         ((uint32_t)data[in_index + 1] << 8) |
                         (uint32_t)data[in_index + 2];

        encoded[out_index++] = k_sequence_base64url_alphabet[(chunk >> 18) & 0x3f];
        encoded[out_index++] = k_sequence_base64url_alphabet[(chunk >> 12) & 0x3f];
        encoded[out_index++] = k_sequence_base64url_alphabet[(chunk >> 6) & 0x3f];
        encoded[out_index++] = k_sequence_base64url_alphabet[chunk & 0x3f];
        in_index += 3;
    }

    if (len - in_index == 1)
    {
        uint32_t chunk = (uint32_t)data[in_index] << 16;

        encoded[out_index++] = k_sequence_base64url_alphabet[(chunk >> 18) & 0x3f];
        encoded[out_index++] = k_sequence_base64url_alphabet[(chunk >> 12) & 0x3f];
    }
    else if (len - in_index == 2)
    {
        uint32_t chunk = ((uint32_t)data[in_index] << 16) |
                         ((uint32_t)data[in_index + 1] << 8);

        encoded[out_index++] = k_sequence_base64url_alphabet[(chunk >> 18) & 0x3f];
        encoded[out_index++] = k_sequence_base64url_alphabet[(chunk >> 12) & 0x3f];
        encoded[out_index++] = k_sequence_base64url_alphabet[(chunk >> 6) & 0x3f];
    }

    encoded[out_index] = '\0';
    return encoded;
}
