#ifndef SEQUENCE_REQUEST_SIGNING_H
#define SEQUENCE_REQUEST_SIGNING_H

#include <stdint.h>

char *sequence_build_wallet_request_preimage(
    const char *endpoint,
    const char *nonce,
    const char *payload
);

char *sequence_wallet_request_preimage_digest_hex(const char *preimage);

char *sequence_wallet_address_from_seckey(const uint8_t seckey32[32]);

char *sequence_sign_wallet_digest_hex_eip191(
    const uint8_t seckey32[32],
    const char *digest_hex
);

char *sequence_sign_wallet_request_preimage(
    const uint8_t seckey32[32],
    const char *preimage
);

char *sequence_build_wallet_authorization_header(
    const char *scope,
    const char *address,
    const char *nonce,
    const char *signature
);

#endif
