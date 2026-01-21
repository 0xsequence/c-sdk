#ifndef SEQUENCE_WALLET_H
#define SEQUENCE_WALLET_H

#include <stdint.h>
#include <stddef.h>

#include <secp256k1.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct sequence_wallet {
        uint8_t seckey[32];
        secp256k1_pubkey pubkey;
    } sequence_wallet_t;

    /*
     * Initialize an existing wallet by generating a random private key
     * and deriving the public key.
     *
     * Returns 1 on success, 0 on failure.
     */
    int sequence_wallet_initialize(sequence_wallet_t *wallet);

    /*
     * Serialize public key to bytes.
     * compressed=1 -> 33 bytes; compressed=0 -> 65 bytes.
     * Returns bytes written (33/65) or 0 on failure.
     */
    size_t sequence_wallet_serialize_pubkey(
        const sequence_wallet_t *wallet,
        uint8_t *out,
        int compressed
    );

    /* Wipe private key material in memory. */
    void sequence_wallet_clear(sequence_wallet_t *wallet);

#ifdef __cplusplus
}
#endif

#endif /* SEQUENCE_WALLET_H */
