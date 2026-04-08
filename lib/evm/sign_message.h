#ifndef SEQUENCE_SIGN_MESSAGE_H
#define SEQUENCE_SIGN_MESSAGE_H

#include <stdint.h>
#include <secp256k1.h>

char *wallet_sign_utf8_message_hex_eip191(
    secp256k1_context *ctx,
    const uint8_t seckey32[32],
    const char *message_utf8
);

char *wallet_message_digest_hex_eip191(const char *message_utf8);

char *wallet_sign_message_hex_eip191(
    secp256k1_context *ctx,
    const uint8_t seckey32[32],
    const char *data_to_sign_utf8
);

#endif
