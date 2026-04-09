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
    waas_complete_auth_request params;
    waas_wallet_complete_auth_request request;
    waas_prepared_request prepared_request;
    waas_error error;
    char *payload = NULL;

    waas_complete_auth_request_init(&params);
    waas_wallet_complete_auth_request_init(&request);
    waas_prepared_request_init(&prepared_request);
    waas_error_init(&error);

    params.identity_type = WAAS_IDENTITY_TYPE_EMAIL;
    params.auth_mode = WAAS_AUTH_MODE_OTP;
    params.verifier = waas_strdup(verifier);
    params.answer = waas_strdup(answer);
    request.complete_auth_request = &params;

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
    waas_complete_auth_request_free(&params);
    return payload;
}

static char *build_commit_verifier_payload(const char *handle)
{
    waas_commit_verifier_request params;
    waas_wallet_commit_verifier_request request;
    waas_prepared_request prepared_request;
    waas_error error;
    char *payload = NULL;

    waas_commit_verifier_request_init(&params);
    waas_wallet_commit_verifier_request_init(&request);
    waas_prepared_request_init(&prepared_request);
    waas_error_init(&error);

    params.identity_type = WAAS_IDENTITY_TYPE_EMAIL;
    params.auth_mode = WAAS_AUTH_MODE_OTP;
    params.has_handle = true;
    params.handle = waas_strdup(handle);
    request.commit_verifier_request = &params;

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
    waas_commit_verifier_request_free(&params);
    return payload;
}

static char *build_create_wallet_payload(waas_wallet_type wallet_type)
{
    waas_create_wallet_request params;
    waas_wallet_create_wallet_request request;
    waas_prepared_request prepared_request;
    waas_error error;
    char *payload = NULL;

    waas_create_wallet_request_init(&params);
    waas_wallet_create_wallet_request_init(&request);
    waas_prepared_request_init(&prepared_request);
    waas_error_init(&error);

    params.wallet_type = wallet_type;
    request.create_wallet_request = &params;

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
    waas_create_wallet_request_free(&params);
    return payload;
}

static char *build_use_wallet_payload(waas_wallet_type wallet_type, long long wallet_index)
{
    waas_use_wallet_request params;
    waas_wallet_use_wallet_request request;
    waas_prepared_request prepared_request;
    waas_error error;
    char *payload = NULL;

    waas_use_wallet_request_init(&params);
    waas_wallet_use_wallet_request_init(&request);
    waas_prepared_request_init(&prepared_request);
    waas_error_init(&error);

    params.wallet_type = wallet_type;
    params.wallet_index = wallet_index;
    request.use_wallet_request = &params;

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
    waas_use_wallet_request_free(&params);
    return payload;
}

static char *build_sign_message_payload(const char *wallet, const char *network, const char *message)
{
    waas_sign_message_request params;
    waas_wallet_sign_message_request request;
    waas_prepared_request prepared_request;
    waas_error error;
    char *payload = NULL;

    waas_sign_message_request_init(&params);
    waas_wallet_sign_message_request_init(&request);
    waas_prepared_request_init(&prepared_request);
    waas_error_init(&error);

    params.network = waas_strdup(network);
    params.wallet = waas_strdup(wallet);
    params.message = waas_strdup(message);
    request.sign_message_request = &params;

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
    waas_sign_message_request_free(&params);
    return payload;
}

static char *build_send_transaction_payload(
    const char *wallet,
    const char *network,
    const char *to,
    const char *value
)
{
    waas_send_transaction_request params;
    waas_wallet_send_transaction_request request;
    waas_prepared_request prepared_request;
    waas_error error;
    char *payload = NULL;

    waas_send_transaction_request_init(&params);
    waas_wallet_send_transaction_request_init(&request);
    waas_prepared_request_init(&prepared_request);
    waas_error_init(&error);

    params.network = waas_strdup(network);
    params.wallet = waas_strdup(wallet);
    params.to = waas_strdup(to);
    params.value = waas_strdup(value);
    params.mode = WAAS_TRANSACTION_MODE_RELAYER;
    request.send_transaction_request = &params;

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
    waas_send_transaction_request_free(&params);
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
        "{\"identityType\":\"Email\",\"authMode\":\"OTP\",\"verifier\":\"verifier-123\",\"answer\":\"0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd\"}";
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
            .expected_payload = "{\"identityType\":\"Email\",\"authMode\":\"OTP\",\"metadata\":{},\"handle\":\"test@example.com\"}",
            .expected_preimage = "POST /rpc/Wallet/CommitVerifier\nnonce: 1710000003\n\n{\"identityType\":\"Email\",\"authMode\":\"OTP\",\"metadata\":{},\"handle\":\"test@example.com\"}",
            .expected_digest = "0x9dfdd24b22829750ea37aea91976359049d911f80bcebbc24f55093581915509",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0x48d263e63ce61f5b0095a85ed8c935694a4ad845553223b1953eaeec1f278aab1b5a65a4f8e331d335aea486a63deee684dfa4c62de5529c5fd03c6d356550131c",
            .expected_header = "Authorization: Ethereum_Secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000003,sig=\"0x48d263e63ce61f5b0095a85ed8c935694a4ad845553223b1953eaeec1f278aab1b5a65a4f8e331d335aea486a63deee684dfa4c62de5529c5fd03c6d356550131c\""
        },
        {
            .name = "use wallet",
            .build_payload = build_use_wallet_payload_from_context,
            .payload_context = &use_wallet_context,
            .nonce = "1710000004",
            .endpoint = "/UseWallet",
            .scope = "@1:test",
            .expected_payload = "{\"walletType\":\"Ethereum_EOA\",\"walletIndex\":0}",
            .expected_preimage = "POST /rpc/Wallet/UseWallet\nnonce: 1710000004\n\n{\"walletType\":\"Ethereum_EOA\",\"walletIndex\":0}",
            .expected_digest = "0x71ab1786fdbae4975163cee47d3501ffb3e0076c426fbcfe1abfcb3bdd0e7ca8",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0x052b6dd4327e7e07bf31d2006fc1dd469f94a6024b3fa6e0cb1a1fc4dfb203d968682959b8fa01d5298c6c4dbb3ae0407f575fe7c1405535d38008d0b6d149551b",
            .expected_header = "Authorization: Ethereum_Secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000004,sig=\"0x052b6dd4327e7e07bf31d2006fc1dd469f94a6024b3fa6e0cb1a1fc4dfb203d968682959b8fa01d5298c6c4dbb3ae0407f575fe7c1405535d38008d0b6d149551b\""
        },
        {
            .name = "create wallet",
            .build_payload = build_create_wallet_payload_from_context,
            .payload_context = &create_wallet_context,
            .nonce = "1710000005",
            .endpoint = "/CreateWallet",
            .scope = "@1:test",
            .expected_payload = "{\"walletType\":\"Ethereum_EOA\"}",
            .expected_preimage = "POST /rpc/Wallet/CreateWallet\nnonce: 1710000005\n\n{\"walletType\":\"Ethereum_EOA\"}",
            .expected_digest = "0xef892d0808cdca87608ea5f59c1134b2d8e9f5979171f90bf5d01a70d45c8188",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0x9847f3dd071ca467441c15247ff445ac99911059fd043947ea90a59346f18c3800a3837f023ed2e31dc24f3d4581e287de1f10f015fbcf157df7fdc99c84f0921b",
            .expected_header = "Authorization: Ethereum_Secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000005,sig=\"0x9847f3dd071ca467441c15247ff445ac99911059fd043947ea90a59346f18c3800a3837f023ed2e31dc24f3d4581e287de1f10f015fbcf157df7fdc99c84f0921b\""
        },
        {
            .name = "sign message",
            .build_payload = build_sign_message_payload_from_context,
            .payload_context = &sign_message_context,
            .nonce = "1710000000",
            .endpoint = "/SignMessage",
            .scope = "@1:test",
            .expected_payload = "{\"network\":\"amoy\",\"wallet\":\"0x1234567890123456789012345678901234567890\",\"message\":\"hello\"}",
            .expected_preimage = "POST /rpc/Wallet/SignMessage\nnonce: 1710000000\n\n{\"network\":\"amoy\",\"wallet\":\"0x1234567890123456789012345678901234567890\",\"message\":\"hello\"}",
            .expected_digest = "0x17036192bfc9ba35197331ee39fc6774386a2dd49c2d47a49ae39e9b75dab65a",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0x33ea7a72ec2d69cd044f0d8cadcbde50aaf9c0e32288824bea74915549543c2e7dc81aa2fa69999d93f5bde0a65efa207aee634e0782f6e489ddcd73ef412eb51b",
            .expected_header = "Authorization: Ethereum_Secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000000,sig=\"0x33ea7a72ec2d69cd044f0d8cadcbde50aaf9c0e32288824bea74915549543c2e7dc81aa2fa69999d93f5bde0a65efa207aee634e0782f6e489ddcd73ef412eb51b\""
        },
        {
            .name = "send transaction",
            .build_payload = build_send_transaction_payload_from_context,
            .payload_context = &send_transaction_context,
            .nonce = "1710000001",
            .endpoint = "/SendTransaction",
            .scope = "@1:test",
            .expected_payload = "{\"network\":\"amoy\",\"wallet\":\"0x1234567890123456789012345678901234567890\",\"to\":\"0xE5E8B483FfC05967FcFed58cc98D053265af6D99\",\"value\":\"1000\",\"mode\":\"Relayer\"}",
            .expected_preimage = "POST /rpc/Wallet/SendTransaction\nnonce: 1710000001\n\n{\"network\":\"amoy\",\"wallet\":\"0x1234567890123456789012345678901234567890\",\"to\":\"0xE5E8B483FfC05967FcFed58cc98D053265af6D99\",\"value\":\"1000\",\"mode\":\"Relayer\"}",
            .expected_digest = "0x0af11e533aafd6de32e5469bc719dea6b263322d759b48ba748915af20910399",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0xa5d41fb0a5ecc537b6e31f333bac9e13133bd0c144d18bc650b96d19a2e7804b4e87416cc2f92cf2a0b0fb142015de17d3084635f50760a6ea5463addea933f31b",
            .expected_header = "Authorization: Ethereum_Secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000001,sig=\"0xa5d41fb0a5ecc537b6e31f333bac9e13133bd0c144d18bc650b96d19a2e7804b4e87416cc2f92cf2a0b0fb142015de17d3084635f50760a6ea5463addea933f31b\""
        },
        {
            .name = "complete auth",
            .build_payload = build_complete_auth_payload_from_context,
            .payload_context = &complete_auth_context,
            .nonce = "1710000002",
            .endpoint = "/CompleteAuth",
            .scope = "@1:test",
            .expected_answer = "0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd",
            .expected_payload = "{\"identityType\":\"Email\",\"authMode\":\"OTP\",\"verifier\":\"verifier-123\",\"answer\":\"0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd\"}",
            .expected_preimage = "POST /rpc/Wallet/CompleteAuth\nnonce: 1710000002\n\n{\"identityType\":\"Email\",\"authMode\":\"OTP\",\"verifier\":\"verifier-123\",\"answer\":\"0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd\"}",
            .expected_digest = "0x804fc970f4bbec10e17544caeb9d643f1024ad1665d6ace7243422d78b60b0c8",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0xecd2af47b35ad15109d44309888787131c36c603d0a9500d7b4c1eaf33231d0c2c6d26270ee0fde89d037815697a08649fdb50a36916b3b35baf3613728e87e11b",
            .expected_header = "Authorization: Ethereum_Secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000002,sig=\"0xecd2af47b35ad15109d44309888787131c36c603d0a9500d7b4c1eaf33231d0c2c6d26270ee0fde89d037815697a08649fdb50a36916b3b35baf3613728e87e11b\""
        }
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
        run_signing_vector(&vectors[i]);
    }

    test_complete_auth_answer_hash_vector();
    printf("sequence_request_signing_test passed\n");
    return 0;
}
