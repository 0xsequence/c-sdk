#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "generated/waas/waas.gen.h"
#include "wallet/sequence_request_signing.h"
#include "evm/keccak256.h"
#include "utils/hex_utils.h"
#include "utils/string_utils.h"

static const uint8_t test_seckey[32] = {
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11
};

static void expect_string(const char *label, const char *actual, const char *expected)
{
    if (!actual || strcmp(actual, expected) != 0) {
        fprintf(stderr, "%s mismatch\nexpected: %s\nactual:   %s\n",
            label,
            expected ? expected : "(null)",
            actual ? actual : "(null)");
        exit(1);
    }
}

static void expect_prepared_request_metadata(
    const char *label,
    const waas_prepared_request *prepared_request,
    const char *expected_method,
    const char *expected_path,
    const char *expected_content_type
)
{
    char field_label[128];

    if (!prepared_request) {
        fprintf(stderr, "%s prepared request missing\n", label);
        exit(1);
    }

    snprintf(field_label, sizeof(field_label), "%s method", label);
    expect_string(field_label, prepared_request->http_method, expected_method);

    snprintf(field_label, sizeof(field_label), "%s path", label);
    expect_string(field_label, prepared_request->path, expected_path);

    snprintf(field_label, sizeof(field_label), "%s content type", label);
    expect_string(field_label, prepared_request->content_type, expected_content_type);
}

static char *copy_prepared_body(const waas_prepared_request *prepared_request)
{
    char *payload;

    if (!prepared_request || !prepared_request->body) {
        fprintf(stderr, "prepared request body missing\n");
        exit(1);
    }

    payload = waas_strdup(prepared_request->body);
    if (!payload) {
        fprintf(stderr, "failed to copy prepared request body\n");
        exit(1);
    }

    return payload;
}

static char *build_complete_auth_payload(const char *verifier, const char *answer)
{
    waas_complete_auth_params params;
    waas_wallet_complete_auth_request request;
    waas_prepared_request prepared_request;
    waas_error error;
    char *payload = NULL;

    waas_complete_auth_params_init(&params);
    waas_wallet_complete_auth_request_init(&request);
    waas_prepared_request_init(&prepared_request);
    waas_error_init(&error);

    params.identity_type = WAAS_IDENTITY_TYPE_EMAIL;
    params.auth_mode = WAAS_AUTH_MODE_OTP;
    params.verifier = waas_strdup(verifier);
    params.answer = waas_strdup(answer);
    request.params = &params;

    if (!params.verifier || !params.answer ||
        waas_wallet_complete_auth_prepare_request(&request, &prepared_request, &error) != 0) {
        fprintf(stderr, "failed to prepare complete auth payload: %s\n",
            error.message ? error.message : "unknown error");
        exit(1);
    }

    expect_prepared_request_metadata(
        "complete auth",
        &prepared_request,
        "POST",
        "/rpc/Wallet/CompleteAuth",
        "application/json");

    payload = copy_prepared_body(&prepared_request);

    waas_error_free(&error);
    waas_prepared_request_free(&prepared_request);
    waas_complete_auth_params_free(&params);
    return payload;
}

static char *build_commit_verifier_payload(const char *handle)
{
    waas_commit_verifier_params params;
    waas_wallet_commit_verifier_request request;
    waas_prepared_request prepared_request;
    waas_error error;
    char *payload = NULL;

    waas_commit_verifier_params_init(&params);
    waas_wallet_commit_verifier_request_init(&request);
    waas_prepared_request_init(&prepared_request);
    waas_error_init(&error);

    params.identity_type = WAAS_IDENTITY_TYPE_EMAIL;
    params.auth_mode = WAAS_AUTH_MODE_OTP;
    params.has_handle = true;
    params.handle = waas_strdup(handle);
    request.params = &params;

    if (!params.handle ||
        waas_wallet_commit_verifier_prepare_request(&request, &prepared_request, &error) != 0) {
        fprintf(stderr, "failed to prepare commit verifier payload: %s\n",
            error.message ? error.message : "unknown error");
        exit(1);
    }

    expect_prepared_request_metadata(
        "commit verifier",
        &prepared_request,
        "POST",
        "/rpc/Wallet/CommitVerifier",
        "application/json");

    payload = copy_prepared_body(&prepared_request);

    waas_error_free(&error);
    waas_prepared_request_free(&prepared_request);
    waas_commit_verifier_params_free(&params);
    return payload;
}

static char *build_create_wallet_payload(waas_wallet_type wallet_type)
{
    waas_create_wallet_params params;
    waas_wallet_create_wallet_request request;
    waas_prepared_request prepared_request;
    waas_error error;
    char *payload = NULL;

    waas_create_wallet_params_init(&params);
    waas_wallet_create_wallet_request_init(&request);
    waas_prepared_request_init(&prepared_request);
    waas_error_init(&error);

    params.wallet_type = wallet_type;
    request.params = &params;

    if (waas_wallet_create_wallet_prepare_request(&request, &prepared_request, &error) != 0) {
        fprintf(stderr, "failed to prepare create wallet payload: %s\n",
            error.message ? error.message : "unknown error");
        exit(1);
    }

    expect_prepared_request_metadata(
        "create wallet",
        &prepared_request,
        "POST",
        "/rpc/Wallet/CreateWallet",
        "application/json");

    payload = copy_prepared_body(&prepared_request);

    waas_error_free(&error);
    waas_prepared_request_free(&prepared_request);
    waas_create_wallet_params_free(&params);
    return payload;
}

static char *build_use_wallet_payload(waas_wallet_type wallet_type, long long wallet_index)
{
    waas_use_wallet_params params;
    waas_wallet_use_wallet_request request;
    waas_prepared_request prepared_request;
    waas_error error;
    char *payload = NULL;

    waas_use_wallet_params_init(&params);
    waas_wallet_use_wallet_request_init(&request);
    waas_prepared_request_init(&prepared_request);
    waas_error_init(&error);

    params.wallet_type = wallet_type;
    params.wallet_index = wallet_index;
    request.params = &params;

    if (waas_wallet_use_wallet_prepare_request(&request, &prepared_request, &error) != 0) {
        fprintf(stderr, "failed to prepare use wallet payload: %s\n",
            error.message ? error.message : "unknown error");
        exit(1);
    }

    expect_prepared_request_metadata(
        "use wallet",
        &prepared_request,
        "POST",
        "/rpc/Wallet/UseWallet",
        "application/json");

    payload = copy_prepared_body(&prepared_request);

    waas_error_free(&error);
    waas_prepared_request_free(&prepared_request);
    waas_use_wallet_params_free(&params);
    return payload;
}

static char *build_sign_message_payload(const char *wallet, const char *network, const char *message)
{
    waas_sign_message_params params;
    waas_wallet_sign_message_request request;
    waas_prepared_request prepared_request;
    waas_error error;
    char *payload = NULL;

    waas_sign_message_params_init(&params);
    waas_wallet_sign_message_request_init(&request);
    waas_prepared_request_init(&prepared_request);
    waas_error_init(&error);

    params.network = waas_strdup(network);
    params.wallet = waas_strdup(wallet);
    params.message = waas_strdup(message);
    request.params = &params;

    if ((network && !params.network) ||
        (wallet && !params.wallet) ||
        (message && !params.message) ||
        waas_wallet_sign_message_prepare_request(&request, &prepared_request, &error) != 0) {
        fprintf(stderr, "failed to prepare sign message payload: %s\n",
            error.message ? error.message : "unknown error");
        exit(1);
    }

    expect_prepared_request_metadata(
        "sign message",
        &prepared_request,
        "POST",
        "/rpc/Wallet/SignMessage",
        "application/json");

    payload = copy_prepared_body(&prepared_request);

    waas_error_free(&error);
    waas_prepared_request_free(&prepared_request);
    waas_sign_message_params_free(&params);
    return payload;
}

static char *build_send_transaction_payload(
    const char *wallet,
    const char *network,
    const char *to,
    const char *value
)
{
    waas_send_transaction_params params;
    waas_wallet_send_transaction_request request;
    waas_prepared_request prepared_request;
    waas_error error;
    char *payload = NULL;

    waas_send_transaction_params_init(&params);
    waas_wallet_send_transaction_request_init(&request);
    waas_prepared_request_init(&prepared_request);
    waas_error_init(&error);

    params.network = waas_strdup(network);
    params.wallet = waas_strdup(wallet);
    params.to = waas_strdup(to);
    params.value = waas_strdup(value);
    params.mode = WAAS_TRANSACTION_MODE_RELAYER;
    request.params = &params;

    if ((network && !params.network) ||
        (wallet && !params.wallet) ||
        (to && !params.to) ||
        (value && !params.value) ||
        waas_wallet_send_transaction_prepare_request(&request, &prepared_request, &error) != 0) {
        fprintf(stderr, "failed to prepare send transaction payload: %s\n",
            error.message ? error.message : "unknown error");
        exit(1);
    }

    expect_prepared_request_metadata(
        "send transaction",
        &prepared_request,
        "POST",
        "/rpc/Wallet/SendTransaction",
        "application/json");

    payload = copy_prepared_body(&prepared_request);

    waas_error_free(&error);
    waas_prepared_request_free(&prepared_request);
    waas_send_transaction_params_free(&params);
    return payload;
}

typedef char *(*build_payload_fn)(const void *ctx);

typedef struct {
    const char *handle;
} commit_verifier_payload_context;

typedef struct {
    waas_wallet_type wallet_type;
    long long wallet_index;
} use_wallet_payload_context;

typedef struct {
    waas_wallet_type wallet_type;
} create_wallet_payload_context;

typedef struct {
    const char *wallet;
    const char *network;
    const char *message;
} sign_message_payload_context;

typedef struct {
    const char *wallet;
    const char *network;
    const char *to;
    const char *value;
} send_transaction_payload_context;

typedef struct {
    const char *challenge;
    const char *code;
    const char *verifier;
} complete_auth_payload_context;

typedef struct {
    const char *name;
    build_payload_fn build_payload;
    const void *payload_context;
    const char *nonce;
    const char *endpoint;
    const char *scope;
    const char *expected_answer;
    const char *expected_payload;
    const char *expected_preimage;
    const char *expected_digest;
    const char *expected_address;
    const char *expected_signature;
    const char *expected_header;
} signing_vector;

static char *build_commit_verifier_payload_from_context(const void *ctx)
{
    const commit_verifier_payload_context *payload_context = ctx;
    return build_commit_verifier_payload(payload_context->handle);
}

static char *build_use_wallet_payload_from_context(const void *ctx)
{
    const use_wallet_payload_context *payload_context = ctx;
    return build_use_wallet_payload(
        payload_context->wallet_type,
        payload_context->wallet_index);
}

static char *build_create_wallet_payload_from_context(const void *ctx)
{
    const create_wallet_payload_context *payload_context = ctx;
    return build_create_wallet_payload(payload_context->wallet_type);
}

static char *build_sign_message_payload_from_context(const void *ctx)
{
    const sign_message_payload_context *payload_context = ctx;
    return build_sign_message_payload(
        payload_context->wallet,
        payload_context->network,
        payload_context->message);
}

static char *build_send_transaction_payload_from_context(const void *ctx)
{
    const send_transaction_payload_context *payload_context = ctx;
    return build_send_transaction_payload(
        payload_context->wallet,
        payload_context->network,
        payload_context->to,
        payload_context->value);
}

static char *build_complete_auth_answer(const complete_auth_payload_context *payload_context)
{
    uint8_t digest[32];
    char *pre_hash_answer = concat_malloc(payload_context->challenge, payload_context->code);
    char *answer;

    if (!pre_hash_answer) {
        fprintf(stderr, "failed to build complete auth answer input\n");
        exit(1);
    }

    keccak256((const uint8_t *)pre_hash_answer, strlen(pre_hash_answer), digest);
    answer = bytes_to_hex(digest, sizeof(digest));
    free(pre_hash_answer);
    return answer;
}

static char *build_complete_auth_payload_from_context(const void *ctx)
{
    const complete_auth_payload_context *payload_context = ctx;
    char *answer = build_complete_auth_answer(payload_context);
    char *payload = build_complete_auth_payload(payload_context->verifier, answer);
    free(answer);
    return payload;
}

static void run_signing_vector(const signing_vector *vector)
{
    char label[128];
    char *answer = NULL;
    char *payload = vector->build_payload(vector->payload_context);
    char *preimage = sequence_build_wallet_request_preimage(
        vector->endpoint,
        vector->nonce,
        payload);
    char *digest = sequence_wallet_request_preimage_digest_hex(preimage);
    char *address = sequence_wallet_address_from_seckey(test_seckey);
    char *signature = sequence_sign_wallet_digest_hex_eip191(test_seckey, digest);
    char *signature_from_preimage = sequence_sign_wallet_request_preimage(test_seckey, preimage);
    char *header = sequence_build_wallet_authorization_header(
        vector->scope,
        address,
        vector->nonce,
        signature);

    if (vector->expected_answer) {
        answer = build_complete_auth_answer(vector->payload_context);
        snprintf(label, sizeof(label), "%s answer", vector->name);
        expect_string(label, answer, vector->expected_answer);
    }

    snprintf(label, sizeof(label), "%s payload", vector->name);
    expect_string(label, payload, vector->expected_payload);
    snprintf(label, sizeof(label), "%s preimage", vector->name);
    expect_string(label, preimage, vector->expected_preimage);
    snprintf(label, sizeof(label), "%s digest", vector->name);
    expect_string(label, digest, vector->expected_digest);
    snprintf(label, sizeof(label), "%s address", vector->name);
    expect_string(label, address, vector->expected_address);
    snprintf(label, sizeof(label), "%s signature", vector->name);
    expect_string(label, signature, vector->expected_signature);
    snprintf(label, sizeof(label), "%s signature from preimage", vector->name);
    expect_string(label, signature_from_preimage, vector->expected_signature);
    snprintf(label, sizeof(label), "%s header", vector->name);
    expect_string(label, header, vector->expected_header);

    free(answer);
    free(payload);
    free(preimage);
    free(digest);
    free(address);
    free(signature);
    free(signature_from_preimage);
    free(header);
}

static void test_complete_auth_answer_hash_vector(void)
{
    static const complete_auth_payload_context payload_context = {
        .challenge = "challenge",
        .code = "123456",
        .verifier = "verifier-123"
    };
    const char *expected_answer =
        "0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd";
    const char *expected_payload =
        "{\"params\":{\"identityType\":\"Email\",\"authMode\":\"OTP\",\"verifier\":\"verifier-123\",\"answer\":\"0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd\"}}";
    char *answer = build_complete_auth_answer(&payload_context);
    char *payload = build_complete_auth_payload(payload_context.verifier, answer);

    expect_string("complete auth answer hash", answer, expected_answer);
    expect_string("complete auth payload", payload, expected_payload);

    free(answer);
    free(payload);
}

int main(void)
{
    static const commit_verifier_payload_context commit_verifier_context = {
        .handle = "test@example.com"
    };
    static const use_wallet_payload_context use_wallet_context = {
        .wallet_type = WAAS_WALLET_TYPE_ETHEREUM_EOA,
        .wallet_index = 0
    };
    static const create_wallet_payload_context create_wallet_context = {
        .wallet_type = WAAS_WALLET_TYPE_ETHEREUM_EOA
    };
    static const sign_message_payload_context sign_message_context = {
        .wallet = "0x1234567890123456789012345678901234567890",
        .network = "amoy",
        .message = "hello"
    };
    static const send_transaction_payload_context send_transaction_context = {
        .wallet = "0x1234567890123456789012345678901234567890",
        .network = "amoy",
        .to = "0xE5E8B483FfC05967FcFed58cc98D053265af6D99",
        .value = "1000"
    };
    static const complete_auth_payload_context complete_auth_context = {
        .challenge = "challenge",
        .code = "123456",
        .verifier = "verifier-123"
    };
    static const signing_vector vectors[] = {
        {
            .name = "commit verifier",
            .build_payload = build_commit_verifier_payload_from_context,
            .payload_context = &commit_verifier_context,
            .nonce = "1710000003",
            .endpoint = "/CommitVerifier",
            .scope = "@1:test",
            .expected_payload = "{\"params\":{\"identityType\":\"Email\",\"authMode\":\"OTP\",\"metadata\":{},\"handle\":\"test@example.com\"}}",
            .expected_preimage = "POST /rpc/Wallet/CommitVerifier\nnonce: 1710000003\n\n{\"params\":{\"identityType\":\"Email\",\"authMode\":\"OTP\",\"metadata\":{},\"handle\":\"test@example.com\"}}",
            .expected_digest = "0xf39c10b9784a7d291c58b6f53136c014985c90101a08d8b9b3531a4ec90c672f",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0x1f4c5dc95a2c943b61142bd1a839c92e05ea23e80f6b94b58162948ae9a64a467a46a683fb8daadb69b730c1b7aa4fa3cf4f5812de793273fcf40060d8bc3da01c",
            .expected_header = "Authorization: Ethereum_Secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000003,sig=\"0x1f4c5dc95a2c943b61142bd1a839c92e05ea23e80f6b94b58162948ae9a64a467a46a683fb8daadb69b730c1b7aa4fa3cf4f5812de793273fcf40060d8bc3da01c\""
        },
        {
            .name = "use wallet",
            .build_payload = build_use_wallet_payload_from_context,
            .payload_context = &use_wallet_context,
            .nonce = "1710000004",
            .endpoint = "/UseWallet",
            .scope = "@1:test",
            .expected_payload = "{\"params\":{\"walletType\":\"Ethereum_EOA\",\"walletIndex\":0}}",
            .expected_preimage = "POST /rpc/Wallet/UseWallet\nnonce: 1710000004\n\n{\"params\":{\"walletType\":\"Ethereum_EOA\",\"walletIndex\":0}}",
            .expected_digest = "0x28157674c8911273678a3eb23284d730b3c899883be52886104379c187b06ca6",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0xcfb3f9ead0541191b568421dd3b46e8544e141d2fa2fcb5e56a99aefb82a564f5b0846412f50ff711aac9051b144d322fac31a8e59181891136eaf2667c80f891c",
            .expected_header = "Authorization: Ethereum_Secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000004,sig=\"0xcfb3f9ead0541191b568421dd3b46e8544e141d2fa2fcb5e56a99aefb82a564f5b0846412f50ff711aac9051b144d322fac31a8e59181891136eaf2667c80f891c\""
        },
        {
            .name = "create wallet",
            .build_payload = build_create_wallet_payload_from_context,
            .payload_context = &create_wallet_context,
            .nonce = "1710000005",
            .endpoint = "/CreateWallet",
            .scope = "@1:test",
            .expected_payload = "{\"params\":{\"walletType\":\"Ethereum_EOA\"}}",
            .expected_preimage = "POST /rpc/Wallet/CreateWallet\nnonce: 1710000005\n\n{\"params\":{\"walletType\":\"Ethereum_EOA\"}}",
            .expected_digest = "0x1c8d8e5af6e44c973c6619218ebcf89560e59cb4f775acd12b875e2442e98f6f",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0xff6cee523cfd26cc547afc5d9d8961ba025392095884b11333abdc4cdc0e26ac16eb955cea2c444c479fb9ea011a728dd1509049c3744cd3c038bc3a51a714811b",
            .expected_header = "Authorization: Ethereum_Secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000005,sig=\"0xff6cee523cfd26cc547afc5d9d8961ba025392095884b11333abdc4cdc0e26ac16eb955cea2c444c479fb9ea011a728dd1509049c3744cd3c038bc3a51a714811b\""
        },
        {
            .name = "sign message",
            .build_payload = build_sign_message_payload_from_context,
            .payload_context = &sign_message_context,
            .nonce = "1710000000",
            .endpoint = "/SignMessage",
            .scope = "@1:test",
            .expected_payload = "{\"params\":{\"network\":\"amoy\",\"wallet\":\"0x1234567890123456789012345678901234567890\",\"message\":\"hello\"}}",
            .expected_preimage = "POST /rpc/Wallet/SignMessage\nnonce: 1710000000\n\n{\"params\":{\"network\":\"amoy\",\"wallet\":\"0x1234567890123456789012345678901234567890\",\"message\":\"hello\"}}",
            .expected_digest = "0x1da9b6e65c2472c77b51667e01e60268e10215073177cc7d7f192b0fdbb415ec",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0x16ff2ad055498874a0531874821fcaa687168fbc4402e5d446592888b2c29c7b1b968cb1316228c4fc0164e2af60d211947beb5a1ee1e84c20d36c59522269251c",
            .expected_header = "Authorization: Ethereum_Secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000000,sig=\"0x16ff2ad055498874a0531874821fcaa687168fbc4402e5d446592888b2c29c7b1b968cb1316228c4fc0164e2af60d211947beb5a1ee1e84c20d36c59522269251c\""
        },
        {
            .name = "send transaction",
            .build_payload = build_send_transaction_payload_from_context,
            .payload_context = &send_transaction_context,
            .nonce = "1710000001",
            .endpoint = "/SendTransaction",
            .scope = "@1:test",
            .expected_payload = "{\"params\":{\"network\":\"amoy\",\"wallet\":\"0x1234567890123456789012345678901234567890\",\"to\":\"0xE5E8B483FfC05967FcFed58cc98D053265af6D99\",\"value\":\"1000\",\"mode\":\"Relayer\"}}",
            .expected_preimage = "POST /rpc/Wallet/SendTransaction\nnonce: 1710000001\n\n{\"params\":{\"network\":\"amoy\",\"wallet\":\"0x1234567890123456789012345678901234567890\",\"to\":\"0xE5E8B483FfC05967FcFed58cc98D053265af6D99\",\"value\":\"1000\",\"mode\":\"Relayer\"}}",
            .expected_digest = "0x9ba4e8c8a581eb0330ef48a1cf6bad11009db74c3d6bf9e913799416d0e00305",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0xa7ea45d8349c3cdb5c5a4a78937120048c4711cb2e12bf13725423b391d6733e5c0080dbc697fd12a08c5c4faa7bb5182f7f6f956597e1f437b5ec9bbaa164ae1c",
            .expected_header = "Authorization: Ethereum_Secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000001,sig=\"0xa7ea45d8349c3cdb5c5a4a78937120048c4711cb2e12bf13725423b391d6733e5c0080dbc697fd12a08c5c4faa7bb5182f7f6f956597e1f437b5ec9bbaa164ae1c\""
        },
        {
            .name = "complete auth",
            .build_payload = build_complete_auth_payload_from_context,
            .payload_context = &complete_auth_context,
            .nonce = "1710000002",
            .endpoint = "/CompleteAuth",
            .scope = "@1:test",
            .expected_answer = "0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd",
            .expected_payload = "{\"params\":{\"identityType\":\"Email\",\"authMode\":\"OTP\",\"verifier\":\"verifier-123\",\"answer\":\"0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd\"}}",
            .expected_preimage = "POST /rpc/Wallet/CompleteAuth\nnonce: 1710000002\n\n{\"params\":{\"identityType\":\"Email\",\"authMode\":\"OTP\",\"verifier\":\"verifier-123\",\"answer\":\"0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd\"}}",
            .expected_digest = "0x6fe84a6372290cd1e3b68276e1822dbb6021d7576bd6845387c62ee938e1274c",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0x051552b05b0ab8b4cf948803519e2dc63e8d7d0bc9a5637e59253d52eb6b1ca3301234e34441d67963f58015b40e8c43710a5edb1f2db451abbaa90b51a8c7871c",
            .expected_header = "Authorization: Ethereum_Secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000002,sig=\"0x051552b05b0ab8b4cf948803519e2dc63e8d7d0bc9a5637e59253d52eb6b1ca3301234e34441d67963f58015b40e8c43710a5edb1f2db451abbaa90b51a8c7871c\""
        }
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
        run_signing_vector(&vectors[i]);
    }

    test_complete_auth_answer_hash_vector();
    printf("sequence_request_signing_test passed\n");
    return 0;
}
