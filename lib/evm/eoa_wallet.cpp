#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <secp256k1.h>

// OpenSSL for SHA-256
#include <openssl/sha.h>

typedef struct {
    uint8_t seckey[32];
    secp256k1_pubkey pubkey;
} eoa_wallet_t;

static void die(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
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
        SHA256(buf, sizeof(buf), digest);

        if (secp256k1_ec_seckey_verify(ctx, digest)) {
            memcpy(seckey_out, digest, 32);
            // best effort wipe
            OPENSSL_cleanse(digest, sizeof(digest));
            return;
        }
    }

    die("Failed to derive a valid secret key (unexpected)");
}

static void wallet_init_random(eoa_wallet_t *w, secp256k1_context *ctx) {
    // Match your pattern: generate a 64-byte seed, then derive the key.
    uint8_t seed64[64];
    os_random_bytes(seed64, sizeof(seed64));

    derive_seckey_from_seed64(ctx, seed64, w->seckey);

    if (!secp256k1_ec_pubkey_create(ctx, &w->pubkey, w->seckey)) {
        die("secp256k1_ec_pubkey_create failed");
    }

    // best effort wipe seed
    OPENSSL_cleanse(seed64, sizeof(seed64));
}

static void hexprint(const char *label, const uint8_t *buf, size_t len) {
    printf("%s", label);
    for (size_t i = 0; i < len; i++) printf("%02x", buf[i]);
    printf("\n");
}