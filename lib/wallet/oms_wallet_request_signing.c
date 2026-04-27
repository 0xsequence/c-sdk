#include "oms_wallet_request_signing.h"

#include <string.h>
#include <stdlib.h>

#include "evm/eoa_wallet.h"
#include "evm/keccak256.h"
#include "evm/sign_message.h"
#include "utils/hex_utils.h"
#include "utils/string_utils.h"

char *oms_wallet_build_wallet_request_preimage(
    const char *endpoint,
    const char *nonce,
    const char *payload
)
{
    const char *tmpl = "POST /rpc/Wallet{0}\nnonce: {1}\n\n{2}";
    const char *args[] = {endpoint, nonce, payload};
    return format_placeholders(tmpl, args, 3);
}

char *oms_wallet_request_preimage_digest_hex(const char *preimage)
{
    uint8_t digest[32];

    if (!preimage) {
        return NULL;
    }

    keccak256((const uint8_t *)preimage, strlen(preimage), digest);
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

char *oms_wallet_sign_wallet_request_preimage(
    const uint8_t seckey32[32],
    const char *preimage
)
{
    char *digest_hex;
    char *signature;

    digest_hex = oms_wallet_request_preimage_digest_hex(preimage);
    if (!digest_hex) {
        return NULL;
    }

    signature = oms_wallet_sign_wallet_digest_hex_eip191(seckey32, digest_hex);
    free(digest_hex);
    return signature;
}

char *oms_wallet_build_wallet_authorization_header(
    const char *scope,
    const char *address,
    const char *nonce,
    const char *signature
)
{
    const char *header_template =
        "Authorization: ethereum-secp256k1 scope=\"{0}\",cred=\"{1}\",nonce={2},sig=\"{3}\"";
    const char *header_args[] = {scope, address, nonce, signature};
    return format_placeholders(header_template, header_args, 4);
}
