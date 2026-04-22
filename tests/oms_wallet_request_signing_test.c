#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "generated/waas/waas.gen.h"
#include "utils/base64url.h"
#include "utils/sha256.h"
#include "utils/string_utils.h"
#include "wallet/oms_wallet_request_signing.h"

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

    params.type = wallet_type;
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

static char *build_use_wallet_payload(const char *wallet_id)
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

    params.wallet_id = waas_strdup(wallet_id);
    request.use_wallet_request = &params;

    if ((wallet_id && !params.wallet_id) ||
        waas_wallet_use_wallet_prepare_request(&request, &prepared_request, &error) != 0) {
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

static char *build_sign_message_payload(const char *wallet_id, const char *network, const char *message)
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
    params.wallet_id = waas_strdup(wallet_id);
    params.message = waas_strdup(message);
    request.sign_message_request = &params;

    if ((network && !params.network) ||
        (wallet_id && !params.wallet_id) ||
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
    const char *wallet_id,
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
    params.wallet_id = waas_strdup(wallet_id);
    params.to = waas_strdup(to);
    params.value = waas_strdup(value);
    params.mode = WAAS_TRANSACTION_MODE_RELAYER;
    request.send_transaction_request = &params;

    if ((network && !params.network) ||
        (wallet_id && !params.wallet_id) ||
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
    const char *wallet_id;
} use_wallet_payload_context;

typedef struct {
    waas_wallet_type wallet_type;
} create_wallet_payload_context;

typedef struct {
    const char *wallet_id;
    const char *network;
    const char *message;
} sign_message_payload_context;

typedef struct {
    const char *wallet_id;
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
    return build_use_wallet_payload(payload_context->wallet_id);
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
        payload_context->wallet_id,
        payload_context->network,
        payload_context->message);
}

static char *build_send_transaction_payload_from_context(const void *ctx)
{
    const send_transaction_payload_context *payload_context = ctx;
    return build_send_transaction_payload(
        payload_context->wallet_id,
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

    oms_wallet_sha256((const uint8_t *)pre_hash_answer, strlen(pre_hash_answer), digest);
    answer = oms_wallet_base64url_encode_unpadded(digest, sizeof(digest));
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
    char *preimage = oms_wallet_build_wallet_request_preimage(
        vector->endpoint,
        vector->nonce,
        payload);
    char *digest = oms_wallet_request_preimage_digest_hex(preimage);
    char *address = oms_wallet_address_from_seckey(test_seckey);
    char *signature = oms_wallet_sign_wallet_digest_hex_eip191(test_seckey, digest);
    char *signature_from_preimage = oms_wallet_sign_wallet_request_preimage(test_seckey, preimage);
    char *header = oms_wallet_build_wallet_authorization_header(
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
        "2oXiHHjzvN3XzdxGxWTK_c9hZf7pom0OovssPvI7q3M";
    const char *expected_payload =
        "{\"identityType\":\"email\",\"authMode\":\"otp\",\"verifier\":\"verifier-123\",\"answer\":\"2oXiHHjzvN3XzdxGxWTK_c9hZf7pom0OovssPvI7q3M\"}";
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
        .wallet_id = "wallet-123"
    };
    static const create_wallet_payload_context create_wallet_context = {
        .wallet_type = WAAS_WALLET_TYPE_ETHEREUM
    };
    static const sign_message_payload_context sign_message_context = {
        .wallet_id = "wallet-123",
        .network = "amoy",
        .message = "hello"
    };
    static const send_transaction_payload_context send_transaction_context = {
        .wallet_id = "wallet-123",
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
            .expected_payload = "{\"identityType\":\"email\",\"authMode\":\"otp\",\"metadata\":{},\"handle\":\"test@example.com\"}",
            .expected_preimage = "POST /rpc/Wallet/CommitVerifier\nnonce: 1710000003\n\n{\"identityType\":\"email\",\"authMode\":\"otp\",\"metadata\":{},\"handle\":\"test@example.com\"}",
            .expected_digest = "0x033ecc5055e7181814097f54aa68fe7edbb5d3139064c5ae1c801ba1cfbecbcd",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0xab6db8389207099165cb01f7748b2451fefc54c29178d9cf71927d59c79c080906e21c50a04205fe7a5c27168377e06f924ae3ba1bcafe0208a1327789229f2d1b",
            .expected_header = "Authorization: ethereum-secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000003,sig=\"0xab6db8389207099165cb01f7748b2451fefc54c29178d9cf71927d59c79c080906e21c50a04205fe7a5c27168377e06f924ae3ba1bcafe0208a1327789229f2d1b\""
        },
        {
            .name = "use wallet",
            .build_payload = build_use_wallet_payload_from_context,
            .payload_context = &use_wallet_context,
            .nonce = "1710000004",
            .endpoint = "/UseWallet",
            .scope = "@1:test",
            .expected_payload = "{\"walletId\":\"wallet-123\"}",
            .expected_preimage = "POST /rpc/Wallet/UseWallet\nnonce: 1710000004\n\n{\"walletId\":\"wallet-123\"}",
            .expected_digest = "0x2e5905f55ee7db83cb3354886c9ea47b3ffda34f979e70019b24a7effb5cecab",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0xc9607e16e7ad54ba8702c1f8977c1a2592561788e943a49841037c0b168f49d62b931ad5ec0d9c77a03749a0b0155406b1ab6ed7f8b8b65b19f8a059a774c3ca1c",
            .expected_header = "Authorization: ethereum-secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000004,sig=\"0xc9607e16e7ad54ba8702c1f8977c1a2592561788e943a49841037c0b168f49d62b931ad5ec0d9c77a03749a0b0155406b1ab6ed7f8b8b65b19f8a059a774c3ca1c\""
        },
        {
            .name = "create wallet",
            .build_payload = build_create_wallet_payload_from_context,
            .payload_context = &create_wallet_context,
            .nonce = "1710000005",
            .endpoint = "/CreateWallet",
            .scope = "@1:test",
            .expected_payload = "{\"type\":\"ethereum\"}",
            .expected_preimage = "POST /rpc/Wallet/CreateWallet\nnonce: 1710000005\n\n{\"type\":\"ethereum\"}",
            .expected_digest = "0x1d8cb9d0c5397a5287afa89fc3c6530de756504602692e5f7d7cbdb3ae128e9e",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0x47ace7468b3084862e4434aefa50dc1b7ded03a32c9d8a8bcaad72f18612f72c6600924f8bafd5161c614f68295d0a0318b33819c494f86b3ab243909ec2378e1c",
            .expected_header = "Authorization: ethereum-secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000005,sig=\"0x47ace7468b3084862e4434aefa50dc1b7ded03a32c9d8a8bcaad72f18612f72c6600924f8bafd5161c614f68295d0a0318b33819c494f86b3ab243909ec2378e1c\""
        },
        {
            .name = "sign message",
            .build_payload = build_sign_message_payload_from_context,
            .payload_context = &sign_message_context,
            .nonce = "1710000000",
            .endpoint = "/SignMessage",
            .scope = "@1:test",
            .expected_payload = "{\"network\":\"amoy\",\"walletId\":\"wallet-123\",\"message\":\"hello\"}",
            .expected_preimage = "POST /rpc/Wallet/SignMessage\nnonce: 1710000000\n\n{\"network\":\"amoy\",\"walletId\":\"wallet-123\",\"message\":\"hello\"}",
            .expected_digest = "0x2677991ae3d7458ca5a6332f9a97bd9be243c141588315249d2a3ee8ad63a5b5",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0xdd1846c9b0cf22224e3f4d48d69697cddf85d9cc0d3bc07e2000f056c7dd904f35bfee21ff04a8d1e2f21a914bc1599a2cd66a23e0f352046d811cbb7cd7bf0b1b",
            .expected_header = "Authorization: ethereum-secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000000,sig=\"0xdd1846c9b0cf22224e3f4d48d69697cddf85d9cc0d3bc07e2000f056c7dd904f35bfee21ff04a8d1e2f21a914bc1599a2cd66a23e0f352046d811cbb7cd7bf0b1b\""
        },
        {
            .name = "send transaction",
            .build_payload = build_send_transaction_payload_from_context,
            .payload_context = &send_transaction_context,
            .nonce = "1710000001",
            .endpoint = "/SendTransaction",
            .scope = "@1:test",
            .expected_payload = "{\"network\":\"amoy\",\"walletId\":\"wallet-123\",\"to\":\"0xE5E8B483FfC05967FcFed58cc98D053265af6D99\",\"value\":\"1000\",\"mode\":\"relayer\"}",
            .expected_preimage = "POST /rpc/Wallet/SendTransaction\nnonce: 1710000001\n\n{\"network\":\"amoy\",\"walletId\":\"wallet-123\",\"to\":\"0xE5E8B483FfC05967FcFed58cc98D053265af6D99\",\"value\":\"1000\",\"mode\":\"relayer\"}",
            .expected_digest = "0x0e6f40130b1d5f1f569d41e529062b5fc077beadae4c5abfc06ae62e258d6009",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0x929447ac2d417b51d0f6c18699871736e8e82e3e5c1e9f4e475ace34974127b16adfa4c611155be312f3ee05362095e7a17186ab97a4bdb55338ba33846aac6a1c",
            .expected_header = "Authorization: ethereum-secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000001,sig=\"0x929447ac2d417b51d0f6c18699871736e8e82e3e5c1e9f4e475ace34974127b16adfa4c611155be312f3ee05362095e7a17186ab97a4bdb55338ba33846aac6a1c\""
        },
        {
            .name = "complete auth",
            .build_payload = build_complete_auth_payload_from_context,
            .payload_context = &complete_auth_context,
            .nonce = "1710000002",
            .endpoint = "/CompleteAuth",
            .scope = "@1:test",
            .expected_answer = "2oXiHHjzvN3XzdxGxWTK_c9hZf7pom0OovssPvI7q3M",
            .expected_payload = "{\"identityType\":\"email\",\"authMode\":\"otp\",\"verifier\":\"verifier-123\",\"answer\":\"2oXiHHjzvN3XzdxGxWTK_c9hZf7pom0OovssPvI7q3M\"}",
            .expected_preimage = "POST /rpc/Wallet/CompleteAuth\nnonce: 1710000002\n\n{\"identityType\":\"email\",\"authMode\":\"otp\",\"verifier\":\"verifier-123\",\"answer\":\"2oXiHHjzvN3XzdxGxWTK_c9hZf7pom0OovssPvI7q3M\"}",
            .expected_digest = "0x74fcf06516ebe2767cf034bb4d9e73662cad443a243ab86e1e86bad94857270b",
            .expected_address = "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",
            .expected_signature = "0x925272704a36bdebf2643f884d5270cd5da1a344a0f55c569054757e680381254469b27ed9f816f4727dd5edc235a17ff1dd0c84f495f4dbd0fb291d499d806e1b",
            .expected_header = "Authorization: ethereum-secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000002,sig=\"0x925272704a36bdebf2643f884d5270cd5da1a344a0f55c569054757e680381254469b27ed9f816f4727dd5edc235a17ff1dd0c84f495f4dbd0fb291d499d806e1b\""
        }
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
        run_signing_vector(&vectors[i]);
    }

    test_complete_auth_answer_hash_vector();
    printf("oms_wallet_request_signing_test passed\n");
    return 0;
}
