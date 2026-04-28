#include "oms_wallet_request_signing.h"

#include <stdlib.h>
#include <string.h>

#include "evm/eoa_wallet.h"
#include "evm/keccak256.h"
#include "evm/sign_message.h"
#include "utils/hex_utils.h"
#include "utils/string_utils.h"

static int checked_add_size(size_t *acc, size_t value)
{
    if (!acc || value > SIZE_MAX - *acc) {
        return -1;
    }

    *acc += value;
    return 0;
}

char *oms_wallet_build_authorization_message(
    const char *method,
    const char *path,
    const char *metadata,
    const uint8_t *body,
    size_t body_len,
    size_t *out_message_len
)
{
    char *message;
    size_t method_len;
    size_t path_len;
    size_t metadata_len;
    size_t total_len;
    size_t off = 0;

    if (!method || !path || !metadata || (!body && body_len != 0)) {
        return NULL;
    }

    method_len = strlen(method);
    path_len = strlen(path);
    metadata_len = strlen(metadata);
    total_len = 0;
    if (checked_add_size(&total_len, method_len) != 0 ||
        checked_add_size(&total_len, 1) != 0 ||
        checked_add_size(&total_len, path_len) != 0 ||
        checked_add_size(&total_len, 1) != 0 ||
        checked_add_size(&total_len, metadata_len) != 0 ||
        checked_add_size(&total_len, 2) != 0 ||
        checked_add_size(&total_len, body_len) != 0 ||
        checked_add_size(&total_len, 1) != 0) {
        return NULL;
    }

    message = malloc(total_len);
    if (!message) {
        return NULL;
    }

    memcpy(message + off, method, method_len);
    off += method_len;
    message[off++] = ' ';
    memcpy(message + off, path, path_len);
    off += path_len;
    message[off++] = '\n';
    memcpy(message + off, metadata, metadata_len);
    off += metadata_len;
    message[off++] = '\n';
    message[off++] = '\n';
    if (body_len > 0) {
        memcpy(message + off, body, body_len);
        off += body_len;
    }
    message[off] = '\0';

    if (out_message_len) {
        *out_message_len = total_len - 1;
    }
    return message;
}

char *oms_wallet_request_preimage_digest_hex_bytes(
    const uint8_t *preimage,
    size_t preimage_len
)
{
    static const uint8_t empty_preimage = 0;
    uint8_t digest[32];

    if (!preimage && preimage_len != 0) {
        return NULL;
    }

    if (!preimage) {
        preimage = &empty_preimage;
    }

    keccak256(preimage, preimage_len, digest);
    return bytes_to_hex(digest, sizeof(digest));
}

char *oms_wallet_address_from_seckey(const uint8_t seckey32[32])
{
    eoa_wallet_t wallet;

    if (!seckey32) {
        return NULL;
    }

    if (!eoa_wallet_from_private_key_bytes(&wallet, seckey32)) {
        return NULL;
    }

    char *address = eoa_wallet_get_address(wallet.ctx, &wallet.pubkey);
    eoa_wallet_destroy(&wallet);
    return address;
}

char *oms_wallet_sign_wallet_digest_hex_eip191(
    const uint8_t seckey32[32],
    const char *digest_hex
)
{
    secp256k1_context *ctx;
    char *signature;

    if (!seckey32 || !digest_hex) {
        return NULL;
    }

    ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    if (!ctx) {
        return NULL;
    }

    signature = wallet_sign_utf8_message_hex_eip191(ctx, seckey32, digest_hex);
    secp256k1_context_destroy(ctx);
    return signature;
}

char *oms_wallet_sign_wallet_request_preimage_bytes(
    const uint8_t seckey32[32],
    const uint8_t *preimage,
    size_t preimage_len
)
{
    char *digest_hex;
    char *signature;

    digest_hex = oms_wallet_request_preimage_digest_hex_bytes(preimage, preimage_len);
    if (!digest_hex) {
        return NULL;
    }

    signature = oms_wallet_sign_wallet_digest_hex_eip191(seckey32, digest_hex);
    free(digest_hex);
    return signature;
}

char *oms_wallet_build_wallet_authorization_header(
    const char *key_type,
    const char *scope,
    const char *credential,
    const char *nonce,
    const char *signature
)
{
    const char *header_template =
        "Authorization: {0} scope=\"{1}\",cred=\"{2}\",nonce={3},sig=\"{4}\"";
    const char *header_args[] = {key_type, scope, credential, nonce, signature};
    return format_placeholders(header_template, header_args, 5);
}
