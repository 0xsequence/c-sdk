#include "eoa_wallet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../utils/hex_utils.h"

#include "evm/keccak256.h"

static int random32(uint8_t out[32]) {
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) return 0;
    if (fread(out, 1, 32, f) != 32) {
        fclose(f);
        return 0;
    }
    fclose(f);
    return 1;
}

int eoa_wallet_initialize(eoa_wallet_t *wallet) {
    if (!wallet) return 0;
    memset(wallet, 0, sizeof(*wallet));

    wallet->ctx =
        secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

    if (!wallet->ctx) return 0;

    /* 1) Generate valid secret key */
    do {
        if (!random32(wallet->seckey)) {
            eoa_wallet_destroy(wallet);
            return 0;
        }
    } while (!secp256k1_ec_seckey_verify(wallet->ctx, wallet->seckey));

    /* 2) Derive public key */
    int ok = secp256k1_ec_pubkey_create(wallet->ctx, &wallet->pubkey, wallet->seckey);

    //secp256k1_context_destroy(wallet->ctx);

    if (!ok) {
        eoa_wallet_destroy(wallet);
        return 0;
    }

    return 1;
}

size_t eoa_wallet_serialize_pubkey(
    const eoa_wallet_t *wallet,
    uint8_t *out,
    int compressed
) {
    if (!wallet || !out) return 0;

    secp256k1_context *ctx =
        secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    if (!ctx) return 0;

    unsigned int flags = compressed ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED;
    size_t outlen = compressed ? 33 : 65;

    int ok = secp256k1_ec_pubkey_serialize(ctx, out, &outlen, &wallet->pubkey, flags);
    secp256k1_context_destroy(ctx);

    return ok ? outlen : 0;
}

int eoa_wallet_from_private_key_bytes(eoa_wallet_t *wallet, const uint8_t seckey32[32]) {
    if (!wallet || !seckey32)
    {
        fprintf(stderr, "invalid seckey\n");
        return 0;
    }

    memset(wallet, 0, sizeof(*wallet));

    wallet->ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    if (!wallet->ctx)
    {
        fprintf(stderr, "failed to create context\n");
        return 0;
    }

    // Copy secret key into wallet
    memcpy(wallet->seckey, seckey32, 32);

    // Verify secret key is valid for secp256k1
    if (!secp256k1_ec_seckey_verify(wallet->ctx, wallet->seckey)) {
        fprintf(stderr, "failed to verify seckey\n");
        eoa_wallet_destroy(wallet);
        return 0;
    }

    // Derive public key
    if (!secp256k1_ec_pubkey_create(wallet->ctx, &wallet->pubkey, wallet->seckey)) {
        fprintf(stderr, "failed to create pubkey\n");
        eoa_wallet_destroy(wallet);
        return 0;
    }

    return 1;
}

char *eoa_wallet_get_address(
    secp256k1_context *ctx,
    secp256k1_pubkey *pubkey)
{
    uint8_t pubkey_ser[65];
    size_t pubkey_len = 65;

    secp256k1_ec_pubkey_serialize(
        ctx, pubkey_ser, &pubkey_len, pubkey,
        SECP256K1_EC_UNCOMPRESSED
    );

    // Keccak-256 of X||Y
    uint8_t hash[32];
    keccak256(pubkey_ser + 1, 64, hash);

    // Last 20 bytes = address
    uint8_t address[20];
    memcpy(address, hash + 12, 20);

    // Hex encode
    char *addr_hex = bytes_to_hex(address, 20);
    return addr_hex;
}

void eoa_wallet_destroy(eoa_wallet_t *wallet) {
    if (!wallet) return;
    if (wallet->ctx) secp256k1_context_destroy(wallet->ctx);
    memset(wallet, 0, sizeof(*wallet));
}
