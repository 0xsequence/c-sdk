#ifndef OMS_WALLET_BASE64URL_H
#define OMS_WALLET_BASE64URL_H

#include <stddef.h>
#include <stdint.h>

size_t oms_wallet_base64url_unpadded_encoded_len(size_t len);
char *oms_wallet_base64url_encode_unpadded(const uint8_t *data, size_t len);

#endif
