#include "eoa_wallet.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <secp256k1.h>
#include <mbedtls/md.h>
#include <mbedtls/platform_util.h>

static void die(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

static void sha256_mbedtls(const uint8_t *buf, size_t len, uint8_t digest[32]) {
    const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!info) die("MBEDTLS_MD_SHA256 not available (check config)");

    int rc = mbedtls_md(info, buf, len, digest);
    if (rc != 0) die("mbedtls_md(SHA256) failed");
}

// Simple Unix-like secure RNG. For production portability:
// - Linux: getrandom(2)
// - macOS/BSD: arc4random_buf
// - Windows: BCryptGenRandom
static void os_random_bytes(uint8_t *out, size_t n) {
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) die("Failed to open /dev/urandom");
    if (fread(out, 1, n, f) != n) die("Failed to read random bytes");
    fclose(f);
}

// Derive a valid secp256k1 seckey from a 64-byte seed.
// We do: seckey = SHA256(seed || counter) until secp256k1 accepts it.
static void derive_seckey_from_seed64(
    secp256k1_context *ctx,
    const uint8_t seed64[64],
    uint8_t seckey_out[32]
) {
    uint8_t buf[65];
    memcpy(buf, seed64, 64);

    for (uint32_t counter = 0; counter < 100000; counter++) {
        buf[64] = (uint8_t)(counter & 0xFF);

        uint8_t digest[32];
        sha256_mbedtls(buf, sizeof(buf), digest);

        if (secp256k1_ec_seckey_verify(ctx, digest)) {
            memcpy(seckey_out, digest, 32);
            // best effort wipe
            mbedtls_platform_zeroize(digest, sizeof(digest));
            return;
        }
    }

    die("Failed to derive a valid secret key (unexpected)");
}

int eoa_wallet_init_random(eoa_wallet_t *wallet, secp256k1_context *ctx) {
    if (!wallet || !ctx) return 0;
    memset(wallet, 0, sizeof(*wallet));

    /* 1) Generate valid private key */
    do {
        random32(wallet->seckey);
    } while (!secp256k1_ec_seckey_verify(ctx, wallet->seckey));

    /* 2) Derive pubkey */
    if (!secp256k1_ec_pubkey_create(ctx, &wallet->pubkey, wallet->seckey)) {
        eoa_wallet_clear(wallet);
        return 0;
    }

    /* 3) Serialize uncompressed pubkey (65 bytes: 0x04 || X || Y) */
    uint8_t pub65[65];
    size_t pub65_len = sizeof(pub65);
    if (!secp256k1_ec_pubkey_serialize(
            ctx, pub65, &pub65_len, &wallet->pubkey, SECP256K1_EC_UNCOMPRESSED)) {
        eoa_wallet_clear(wallet);
        return 0;
            }

    /* 4) Compute address = last 20 bytes of hash(pubkey[1..64]) */
    uint8_t hash[32];
    if (!sha3_256(hash, pub65 + 1, 64)) {
        eoa_wallet_clear(wallet);
        return 0;
    }

    memcpy(wallet->address, hash + 12, 20);
    return 1;
}

static void hexprint(const char *label, const uint8_t *buf, size_t len) {
    printf("%s", label);
    for (size_t i = 0; i < len; i++) printf("%02x", buf[i]);
    printf("\n");
}