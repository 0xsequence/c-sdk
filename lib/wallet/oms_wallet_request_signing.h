#ifndef OMS_WALLET_REQUEST_SIGNING_H
#define OMS_WALLET_REQUEST_SIGNING_H

#include <stddef.h>
#include <stdint.h>

char *oms_wallet_build_authorization_message(
    const char *method,
    const char *path,
    const char *metadata,
    const uint8_t *body,
    size_t body_len,
    size_t *out_message_len
);

char *oms_wallet_request_preimage_digest_hex_bytes(
    const uint8_t *preimage,
    size_t preimage_len
);

char *oms_wallet_address_from_seckey(const uint8_t seckey32[32]);

char *oms_wallet_sign_wallet_digest_hex_eip191(
    const uint8_t seckey32[32],
    const char *digest_hex
);

char *oms_wallet_sign_wallet_request_preimage_bytes(
    const uint8_t seckey32[32],
    const uint8_t *preimage,
    size_t preimage_len
);

char *oms_wallet_build_wallet_authorization_header(
    const char *key_type,
    const char *scope,
    const char *credential,
    const char *nonce,
    const char *signature
);

#endif
