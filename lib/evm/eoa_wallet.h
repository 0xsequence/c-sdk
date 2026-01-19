#ifndef EOA_WALLET_H
#define EOA_WALLET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include <secp256k1.h>

    /*
     * EOAWallet
     * ---------
     * Equivalent to:
     *   privKey = ECPrivKey.Create(seed);
     *   pubKey  = privKey.CreatePubKey();
     *
     * - seckey: 32-byte secp256k1 private key
     * - pubkey: secp256k1 public key object
     */
    typedef struct {
        uint8_t seckey[32];
        secp256k1_pubkey pubkey;
    } eoa_wallet_t;

    /*
     * Initialize a new wallet with a randomly generated private key.
     *
     * - Uses OS CSPRNG
     * - Derives a valid secp256k1 private key
     * - Computes the corresponding public key
     *
     * Returns:
     *   1 on success
     *   0 on failure
     *
     * The caller owns the wallet memory.
     */
    int eoa_wallet_init_random(
        eoa_wallet_t *wallet,
        secp256k1_context *ctx
    );

    /*
     * Serialize the public key.
     *
     * Parameters:
     *   compressed = 1  -> 33-byte compressed form
     *   compressed = 0  -> 65-byte uncompressed form
     *
     * Returns:
     *   number of bytes written (33 or 65), or 0 on failure
     */
    size_t eoa_wallet_serialize_pubkey(
        const eoa_wallet_t *wallet,
        secp256k1_context *ctx,
        uint8_t *out,
        int compressed
    );

    /*
     * Securely wipe the private key from memory.
     * Call when you no longer need the wallet.
     */
    void eoa_wallet_clear(eoa_wallet_t *wallet);

#ifdef __cplusplus
}
#endif

#endif /* EOA_WALLET_H */
