#ifndef OMS_WALLET_REQUEST_SIGNING_H
#define OMS_WALLET_REQUEST_SIGNING_H

#include <stdint.h>

char *oms_wallet_build_wallet_request_preimage(
    const char *endpoint,
    const char *nonce,
    const char *payload
);

char *oms_wallet_request_preimage_digest_hex(const char *preimage);

char *oms_wallet_address_from_seckey(const uint8_t seckey32[32]);

char *oms_wallet_sign_wallet_digest_hex_eip191(
    const uint8_t seckey32[32],
    const char *digest_hex
);

char *oms_wallet_sign_wallet_request_preimage(
    const uint8_t seckey32[32],
    const char *preimage
);

char *oms_wallet_build_wallet_authorization_header(
    const char *scope,
    const char *address,
    const char *nonce,
    const char *signature
);

#endif
