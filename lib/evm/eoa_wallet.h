#include <stdint.h>
#include <secp256k1.h>

typedef struct {
    uint8_t seckey[32];
    secp256k1_pubkey pubkey;
} eoa_wallet_t;

int eoa_wallet_init_random(
    eoa_wallet_t *wallet,
    secp256k1_context *ctx
);

size_t eoa_wallet_serialize_pubkey(
    const eoa_wallet_t *wallet,
    secp256k1_context *ctx,
    uint8_t *out,
    int compressed
);

void eoa_wallet_clear(eoa_wallet_t *wallet);
