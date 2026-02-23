#include <string.h>
#include <stdio.h>
#include <secp256k1.h>
#include <secp256k1_recovery.h>

#include "keccak256.h"

static void die(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

// --- helper: hex without 0x into existing buffer ---
static void hex_encode_lower(const uint8_t *in, size_t inlen, char *out /* needs 2*inlen */) {
    static const char h[] = "0123456789abcdef";
    for (size_t i = 0; i < inlen; i++) {
        out[i*2+0] = h[(in[i] >> 4) & 0xF];
        out[i*2+1] = h[in[i] & 0xF];
    }
}

char *hex_encode0x_malloc(const uint8_t *buf, size_t len) {
    static const char h[] = "0123456789abcdef";
    char *out = (char *)malloc(2 + len * 2 + 1);
    if (!out) die("malloc failed");
    out[0] = '0';
    out[1] = 'x';
    for (size_t i = 0; i < len; i++) {
        out[2 + i * 2 + 0] = h[(buf[i] >> 4) & 0xF];
        out[2 + i * 2 + 1] = h[buf[i] & 0xF];
    }
    out[2 + len * 2] = '\0';
    return out;
}

/* ------------------------- EIP-191 prefix helper ------------------------- */

static size_t u64_to_dec(char *dst, size_t dstcap, uint64_t v) {
    char tmp[32];
    size_t n = 0;
    do {
        tmp[n++] = (char)('0' + (v % 10));
        v /= 10;
    } while (v && n < sizeof(tmp));

    if (n + 1 > dstcap) return 0;

    // reverse
    for (size_t i = 0; i < n; i++) dst[i] = tmp[n - 1 - i];
    dst[n] = '\0';
    return n;
}

// Builds: "\x19Ethereum Signed Message:\n" + dec(len) + message
// Returns malloc'd buffer and length via out_len.
uint8_t *prefixed_message_malloc(const uint8_t *msg, size_t msg_len, size_t *out_len) {
    static const char prefix[] = "\x19" "Ethereum Signed Message:\n";

    char len_dec[32];
    size_t len_dec_n = u64_to_dec(len_dec, sizeof(len_dec), (uint64_t)msg_len);
    if (len_dec_n == 0) die("u64_to_dec failed");

    size_t total = (sizeof(prefix) - 1) + len_dec_n + msg_len;
    uint8_t *buf = (uint8_t *)malloc(total);
    if (!buf) die("malloc failed");

    size_t off = 0;
    memcpy(buf + off, prefix, sizeof(prefix) - 1); off += (sizeof(prefix) - 1);
    memcpy(buf + off, len_dec, len_dec_n);         off += len_dec_n;
    memcpy(buf + off, msg, msg_len);               off += msg_len;

    *out_len = total;
    return buf;
}

// Sign a 32-byte hash directly (NO hashing/prefixing inside). Returns 0x + r+s+v, with v in {0,1}.
static char *secp256k1_sign_hash65_hex_v01(
    secp256k1_context *ctx,
    const uint8_t seckey32[32],
    const uint8_t hash32[32]
) {
    secp256k1_ecdsa_recoverable_signature rsig;
    if (!secp256k1_ecdsa_sign_recoverable(ctx, &rsig, hash32, seckey32, NULL, NULL)) {
        die("secp256k1_ecdsa_sign_recoverable failed");
    }

    uint8_t compact64[64];
    int recid = 0;
    secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact64, &recid, &rsig);

    // low-s normalize + fix recid like you already do
    secp256k1_ecdsa_signature nsig, nsig_norm;
    secp256k1_ecdsa_recoverable_signature_convert(ctx, &nsig, &rsig);
    int changed = secp256k1_ecdsa_signature_normalize(ctx, &nsig_norm, &nsig);
    if (changed) {
        secp256k1_ecdsa_signature_serialize_compact(ctx, compact64, &nsig_norm);
        recid ^= 1;
    }

    uint8_t sig65[65];
    memcpy(sig65, compact64, 64);
    sig65[64] = (uint8_t)(recid + 27);

    char *hex = hex_encode0x_malloc(sig65, 65);

    memset(compact64, 0, sizeof(compact64));
    memset(sig65, 0, sizeof(sig65));
    return hex;
}

// Signs message bytes (optionally concatenated with chainId bytes) like ethers v5 signMessage.
// Returns signature hex string: 0x + r(32) + s(32) + v(1), with v in {27,28}.
char *wallet_sign_message_hex_eip191(
    secp256k1_context *ctx,
    const uint8_t seckey32[32],
    const char *data_to_sign_utf8
) {
    // 1) digest32 = keccak256(messageBytes)
    uint8_t digest32[32];
    keccak256((const uint8_t*)data_to_sign_utf8, strlen(data_to_sign_utf8), digest32);

    // 2) digestHex ASCII = "0x" + 64 hex chars
    char digestHex[2 + 64 + 1];
    digestHex[0] = '0';
    digestHex[1] = 'x';
    hex_encode_lower(digest32, 32, digestHex + 2);
    digestHex[66] = '\0';

    // 3) prefixedHash = keccak256( EIP-191 prefix with len(digestHex) + digestHex )
    size_t pref_len = 0;
    uint8_t *pref = prefixed_message_malloc((const uint8_t*)digestHex, strlen(digestHex), &pref_len);

    uint8_t prefixedHash32[32];
    keccak256(pref, pref_len, prefixedHash32);

    memset(pref, 0, pref_len);
    free(pref);

    // 4) sign prefixedHash32 directly
    return secp256k1_sign_hash65_hex_v01(ctx, seckey32, prefixedHash32);
}
