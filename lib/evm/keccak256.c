#include "keccak256.h"
#include <string.h>
#include <stdio.h>

#include <secp256k1.h>
#include <secp256k1_recovery.h>

#define KECCAK_ROTL64(x, n) (((x) << (n)) | ((x) >> (64 - (n))))

static const uint64_t keccakf_rndc[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL,
    0x800000000000808aULL, 0x8000000080008000ULL,
    0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008aULL, 0x0000000000000088ULL,
    0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL,
    0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL,
    0x8000000080008081ULL, 0x8000000000008080ULL,
    0x0000000080000001ULL, 0x8000000080008008ULL
};

static const int keccakf_rotc[24] = {
     1,  3,  6, 10, 15, 21, 28, 36, 45, 55,  2, 14,
    27, 41, 56,  8, 25, 43, 62, 18, 39, 61, 20, 44
};

static const int keccakf_piln[24] = {
    10,  7, 11, 17, 18,  3,  5, 16,  8, 21, 24,  4,
    15, 23, 19, 13, 12,  2, 20, 14, 22,  9,  6,  1
};

static void keccakf(uint64_t st[25]) {
    for (int round = 0; round < 24; round++) {
        uint64_t bc[5], t;

        // Theta
        for (int i = 0; i < 5; i++)
            bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];

        for (int i = 0; i < 5; i++) {
            t = bc[(i + 4) % 5] ^ KECCAK_ROTL64(bc[(i + 1) % 5], 1);
            for (int j = 0; j < 25; j += 5)
                st[j + i] ^= t;
        }

        // Rho + Pi
        t = st[1];
        for (int i = 0; i < 24; i++) {
            int j = keccakf_piln[i];
            bc[0] = st[j];
            st[j] = KECCAK_ROTL64(t, keccakf_rotc[i]);
            t = bc[0];
        }

        // Chi
        for (int j = 0; j < 25; j += 5) {
            uint64_t a0 = st[j + 0], a1 = st[j + 1], a2 = st[j + 2], a3 = st[j + 3], a4 = st[j + 4];
            st[j + 0] ^= (~a1) & a2;
            st[j + 1] ^= (~a2) & a3;
            st[j + 2] ^= (~a3) & a4;
            st[j + 3] ^= (~a4) & a0;
            st[j + 4] ^= (~a0) & a1;
        }

        // Iota
        st[0] ^= keccakf_rndc[round];
    }
}

// Keccak-256: rate=136 bytes, capacity=64 bytes, suffix=0x01 (Keccak padding)
void keccak256(const uint8_t *in, size_t inlen, uint8_t out32[32]) {
    uint64_t st[25];
    uint8_t  temp[144];
    const size_t rate = 136;

    memset(st, 0, sizeof(st));

    // Absorb
    while (inlen >= rate) {
        for (size_t i = 0; i < rate; i++) {
            ((uint8_t *)st)[i] ^= in[i];
        }
        keccakf(st);
        in += rate;
        inlen -= rate;
    }

    // Final block + padding
    memset(temp, 0, sizeof(temp));
    if (inlen) memcpy(temp, in, inlen);
    temp[inlen] = 0x01;           // Keccak domain suffix
    temp[rate - 1] |= 0x80;       // multi-rate padding

    for (size_t i = 0; i < rate; i++) {
        ((uint8_t *)st)[i] ^= temp[i];
    }
    keccakf(st);

    // Squeeze 32 bytes
    memcpy(out32, (uint8_t *)st, 32);
}
