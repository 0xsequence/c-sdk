#include <stdint.h>
#include <secp256k1.h>

char *wallet_sign_message_hex_eip191(
    secp256k1_context *ctx,
    const uint8_t seckey32[32],
    const char *data_to_sign_utf8
);