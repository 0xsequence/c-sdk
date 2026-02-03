#include <stdint.h>
#include <secp256k1.h>

char *wallet_sign_message_hex_eip191(
    const uint8_t seckey32[32],
    secp256k1_context *ctx,
    const uint8_t *message, size_t message_len,
    const uint8_t *chainId, size_t chainId_len
);

char *wallet_sign_string_hex_eip191(
    const uint8_t seckey32[32],
    secp256k1_context *ctx,
    const char *utf8_string
);