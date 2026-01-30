#include <string.h>
#include <stdio.h>

#include <secp256k1.h>
#include <secp256k1_recovery.h>

#include "keccak256.h"

static void die(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

static char *hex_encode0x_malloc(const uint8_t *buf, size_t len) {
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
static uint8_t *prefixed_message_malloc(const uint8_t *msg, size_t msg_len, size_t *out_len) {
    static const char prefix[] = "Ethereum Signed Message:";

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

// Signs message bytes (optionally concatenated with chainId bytes) like ethers v5 signMessage.
// Returns signature hex string: 0x + r(32) + s(32) + v(1), with v in {27,28}.
char *wallet_sign_message_hex_eip191(
    const uint8_t seckey32[32],
    secp256k1_context *ctx,
    const uint8_t *message, size_t message_len
) {
    if (!seckey32 || !ctx || (!message && message_len != 0)) die("bad args");

    // 1) Optional message || chainId
    uint8_t *msg2 = NULL;
    size_t msg2_len = message_len;

    // 2) Prefix
    size_t pref_len = 0;
    uint8_t *pref = prefixed_message_malloc(message, message_len, &pref_len);

    // 3) Keccak-256
    uint8_t digest32[32];
    keccak256(pref, pref_len, digest32);

    // cleanup prefix buffers
    memset(pref, 0, pref_len);
    free(pref);
    if (msg2) { memset(msg2, 0, msg2_len); free(msg2); }

    // 4) Sign recoverable
    secp256k1_ecdsa_recoverable_signature rsig;
    if (!secp256k1_ecdsa_sign_recoverable(ctx, &rsig, digest32, seckey32, NULL, NULL)) {
        memset(digest32, 0, sizeof(digest32));
        die("secp256k1_ecdsa_sign_recoverable failed");
    }

    // 5) Serialize r,s and recovery id
    uint8_t compact64[64];
    int recid = 0;
    secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact64, &recid, &rsig);

    // 6) Enforce low-s (Ethereum standard) and fix recid if s was negated
    // Convert to normal sig, normalize, and if changed => recid ^= 1
    secp256k1_ecdsa_signature nsig, nsig_norm;
    secp256k1_ecdsa_recoverable_signature_convert(ctx, &nsig, &rsig);
    int changed = secp256k1_ecdsa_signature_normalize(ctx, &nsig_norm, &nsig);
    if (changed) {
        // Re-serialize normalized signature back to compact64 (r||s)
        secp256k1_ecdsa_signature_serialize_compact(ctx, compact64, &nsig_norm);
        recid ^= 1;
    }

    // 7) Build final signature: r(32) || s(32) || v(1)
    uint8_t sig65[65];
    memcpy(sig65, compact64, 64);
    sig65[64] = (uint8_t)(recid + 27); // ethers-style v

    // 8) Hex encode with 0x prefix
    char *hex = hex_encode0x_malloc(sig65, 65);

    // best-effort wipe stack buffers
    memset(digest32, 0, sizeof(digest32));
    memset(compact64, 0, sizeof(compact64));
    memset(sig65, 0, sizeof(sig65));

    return hex; // caller must free()
}

// Convenience: string treated as UTF-8 bytes (like ethers signMessage(string)).
char *wallet_sign_string_hex_eip191(
    const uint8_t seckey32[32],
    secp256k1_context *ctx,
    const char *utf8_string
) {
    if (!utf8_string) die("null string");
    const uint8_t *msg = (const uint8_t *)utf8_string;
    size_t msg_len = strlen(utf8_string);
    return wallet_sign_message_hex_eip191(seckey32, ctx, msg, msg_len);
}
