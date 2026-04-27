#ifndef OMS_WALLET_EOA_WALLET_H
#define OMS_WALLET_EOA_WALLET_H

#include <stdint.h>
#include <stddef.h>

#include <secp256k1.h>

typedef struct eoa_wallet {
    uint8_t seckey[32];
    secp256k1_pubkey pubkey;
    secp256k1_context *ctx;
} eoa_wallet_t;

/*
 * Initialize an existing wallet by generating a random private key
 * and deriving the public key.
 *
 * Returns 1 on success, 0 on failure.
 */
int eoa_wallet_initialize(eoa_wallet_t *wallet);

/*
 * Serialize public key to bytes.
 * compressed=1 -> 33 bytes; compressed=0 -> 65 bytes.
 * Returns bytes written (33/65) or 0 on failure.
 */
size_t eoa_wallet_serialize_pubkey(
    const eoa_wallet_t *wallet,
    uint8_t *out,
    int compressed
);

int eoa_wallet_from_private_key_bytes(
    eoa_wallet_t *wallet,
    const uint8_t seckey32[32]
);

char *eoa_wallet_get_address(
    secp256k1_context *ctx,
    secp256k1_pubkey *pubkey
);

void eoa_wallet_destroy(eoa_wallet_t *wallet);

#endif
