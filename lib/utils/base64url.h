#ifndef SEQUENCE_BASE64URL_H
#define SEQUENCE_BASE64URL_H

#include <stddef.h>
#include <stdint.h>

size_t sequence_base64url_unpadded_encoded_len(size_t len);
char *sequence_base64url_encode_unpadded(const uint8_t *data, size_t len);

#endif
