#include "sequence_wallet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/crypto.h>

static void random32(uint8_t out[32]) {
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) return;
    if (fread(out, 1, 32, f) != 32) {
        fclose(f);
        return;
    }
    fclose(f);
}

int sequence_wallet_initialize(sequence_wallet_t *wallet) {
    if (!wallet) return 0;
    memset(wallet, 0, sizeof(*wallet));

    secp256k1_context *ctx =
        secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    if (!ctx) return 0;

    /* 1) Generate valid secret key */
    do {
        random32(wallet->seckey);
        /* If random32 failed, seckey might be all zeros; loop will reject it */
    } while (!secp256k1_ec_seckey_verify(ctx, wallet->seckey));

    /* 2) Derive public key */
    int ok = secp256k1_ec_pubkey_create(ctx, &wallet->pubkey, wallet->seckey);

    secp256k1_context_destroy(ctx);

    if (!ok) {
        sequence_wallet_clear(wallet);
        return 0;
    }

    return 1;
}

size_t sequence_wallet_serialize_pubkey(
    const sequence_wallet_t *wallet,
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

void sequence_wallet_clear(sequence_wallet_t *wallet) {
    if (!wallet) return;
    OPENSSL_cleanse(wallet->seckey, sizeof(wallet->seckey));
    memset(&wallet->pubkey, 0, sizeof(wallet->pubkey));
}
