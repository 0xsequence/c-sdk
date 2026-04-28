#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <secp256k1_recovery.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "evm/eoa_wallet.h"
#include "evm/sign_message.h"
#include "generated/waas/waas.gen.h"
#include "utils/hex_utils.h"
#include "wallet/oms_wallet_config.h"
#include "wallet/oms_wallet_internal.h"
#include "wallet/oms_wallet_request_signing.h"

enum {
    SIM_OP_CREATE = 1,
    SIM_OP_DELETE = 2,
    SIM_OP_GET_CREDENTIAL = 3,
    SIM_OP_SIGN = 4,
    SIM_OP_EXIT = 5
};

typedef struct {
    uint32_t op;
    uint32_t len;
} sim_request_header;

typedef struct {
    int32_t status;
    uint32_t len;
} sim_response_header;

typedef struct {
    int fd;
    char credential[128];
    char last_message[512];
    size_t last_message_len;
} sim_provider_ctx;

typedef struct {
    const sim_provider_ctx *provider;
    int send_count;
    char auth_header[768];
} capture_transport_ctx;

static char *dup_string(const char *value)
{
    size_t len;
    char *copy;

    if (!value)
    {
        return NULL;
    }

    len = strlen(value) + 1;
    copy = malloc(len);
    if (!copy)
    {
        return NULL;
    }
    memcpy(copy, value, len);
    return copy;
}

static void fail(const char *message)
{
    fprintf(stderr, "%s\n", message);
    exit(1);
}

static void expect_int(const char *label, int actual, int expected)
{
    if (actual != expected)
    {
        fprintf(stderr, "%s mismatch: expected %d, got %d\n", label, expected, actual);
        exit(1);
    }
}

static void expect_string(const char *label, const char *actual, const char *expected)
{
    if (!actual || strcmp(actual, expected) != 0)
    {
        fprintf(stderr, "%s mismatch\nexpected: %s\nactual:   %s\n",
            label,
            expected ? expected : "(null)",
            actual ? actual : "(null)");
        exit(1);
    }
}

static void expect_contains(const char *label, const char *actual, const char *needle)
{
    if (!actual || !needle || !strstr(actual, needle))
    {
        fprintf(stderr, "%s missing substring\nneedle: %s\nactual: %s\n",
            label,
            needle ? needle : "(null)",
            actual ? actual : "(null)");
        exit(1);
    }
}

static char *copy_between(const char *start, const char *end)
{
    size_t len;
    char *copy;

    if (!start || !end || end < start)
    {
        return NULL;
    }

    len = (size_t)(end - start);
    copy = malloc(len + 1);
    if (!copy)
    {
        return NULL;
    }
    memcpy(copy, start, len);
    copy[len] = '\0';
    return copy;
}

static char *extract_authorization_signature(const char *auth_header)
{
    const char *start;
    const char *end;

    if (!auth_header)
    {
        return NULL;
    }

    start = strstr(auth_header, ",sig=\"");
    if (!start)
    {
        return NULL;
    }
    start += strlen(",sig=\"");
    end = strchr(start, '"');
    return copy_between(start, end);
}

static char *recover_authorization_address(
    const char *signature_hex,
    const uint8_t *message,
    size_t message_len)
{
    char *digest_hex = NULL;
    char *eip191_digest_hex = NULL;
    uint8_t *digest = NULL;
    uint8_t *signature = NULL;
    size_t digest_len = 0;
    size_t signature_len = 0;
    int recid;
    secp256k1_context *ctx = NULL;
    secp256k1_ecdsa_recoverable_signature recoverable;
    secp256k1_pubkey pubkey;
    char *address = NULL;

    digest_hex = oms_wallet_request_preimage_digest_hex_bytes(message, message_len);
    if (!digest_hex)
    {
        goto cleanup;
    }

    eip191_digest_hex = wallet_message_digest_hex_eip191(digest_hex);
    if (!eip191_digest_hex)
    {
        goto cleanup;
    }

    digest = hex_to_bytes(eip191_digest_hex, &digest_len);
    signature = hex_to_bytes(signature_hex, &signature_len);
    if (!digest || digest_len != 32 || !signature || signature_len != 65)
    {
        goto cleanup;
    }

    recid = signature[64] >= 27 ? signature[64] - 27 : signature[64];
    if (recid < 0 || recid > 3)
    {
        goto cleanup;
    }

    ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    if (!ctx)
    {
        goto cleanup;
    }

    if (!secp256k1_ecdsa_recoverable_signature_parse_compact(
            ctx,
            &recoverable,
            signature,
            recid) ||
        !secp256k1_ecdsa_recover(
            ctx,
            &pubkey,
            &recoverable,
            digest))
    {
        goto cleanup;
    }

    address = eoa_wallet_get_address(ctx, &pubkey);

cleanup:
    free(digest_hex);
    free(eip191_digest_hex);
    free(digest);
    free(signature);
    if (ctx)
    {
        secp256k1_context_destroy(ctx);
    }
    return address;
}

static int write_all(int fd, const void *data, size_t len)
{
    const uint8_t *p = data;

    while (len > 0)
    {
        ssize_t n = write(fd, p, len);
        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return -1;
        }
        p += (size_t)n;
        len -= (size_t)n;
    }

    return 0;
}

static int read_all(int fd, void *data, size_t len)
{
    uint8_t *p = data;

    while (len > 0)
    {
        ssize_t n = read(fd, p, len);
        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return -1;
        }
        if (n == 0)
        {
            return -1;
        }
        p += (size_t)n;
        len -= (size_t)n;
    }

    return 0;
}

static int sim_send_request(
    int fd,
    uint32_t op,
    const uint8_t *payload,
    size_t payload_len,
    char **out_payload)
{
    sim_request_header request;
    sim_response_header response;
    char *response_payload = NULL;

    if (payload_len > UINT32_MAX || (!payload && payload_len != 0))
    {
        return EINVAL;
    }

    request.op = op;
    request.len = (uint32_t)payload_len;
    if (write_all(fd, &request, sizeof(request)) != 0 ||
        (payload_len > 0 && write_all(fd, payload, payload_len) != 0) ||
        read_all(fd, &response, sizeof(response)) != 0)
    {
        return EIO;
    }

    if (response.len > 0)
    {
        response_payload = malloc((size_t)response.len + 1);
        if (!response_payload)
        {
            return ENOMEM;
        }
        if (read_all(fd, response_payload, response.len) != 0)
        {
            free(response_payload);
            return EIO;
        }
        response_payload[response.len] = '\0';
    }

    if (out_payload)
    {
        *out_payload = response_payload;
    }
    else
    {
        free(response_payload);
    }

    return response.status;
}

static int child_send_response(int fd, int32_t status, const char *payload, size_t payload_len)
{
    sim_response_header response;

    response.status = status;
    response.len = (uint32_t)payload_len;
    if (write_all(fd, &response, sizeof(response)) != 0)
    {
        return -1;
    }
    if (payload_len > 0 && write_all(fd, payload, payload_len) != 0)
    {
        return -1;
    }
    return 0;
}

static int child_read_payload(int fd, uint32_t len, uint8_t **out)
{
    uint8_t *payload = NULL;

    if (len > 0)
    {
        payload = malloc((size_t)len + 1);
        if (!payload)
        {
            return -1;
        }
        if (read_all(fd, payload, len) != 0)
        {
            free(payload);
            return -1;
        }
        payload[len] = '\0';
    }

    *out = payload;
    return 0;
}

static void secure_element_child_main(int fd)
{
    eoa_wallet_t wallet;
    char *address = NULL;
    int initialized = 0;
    int deleted = 0;

    if (eoa_wallet_initialize(&wallet) == 0)
    {
        _exit(2);
    }
    initialized = 1;
    address = oms_wallet_address_from_seckey(wallet.seckey);
    if (!address)
    {
        _exit(2);
    }

    for (;;)
    {
        sim_request_header request;
        uint8_t *payload = NULL;

        if (read_all(fd, &request, sizeof(request)) != 0)
        {
            break;
        }
        if (child_read_payload(fd, request.len, &payload) != 0)
        {
            break;
        }

        if (request.op == SIM_OP_CREATE)
        {
            child_send_response(fd, 0, "slot-1", strlen("slot-1"));
        }
        else if (request.op == SIM_OP_DELETE)
        {
            deleted = 1;
            child_send_response(fd, 0, NULL, 0);
        }
        else if (request.op == SIM_OP_GET_CREDENTIAL)
        {
            char response[192];
            size_t key_type_len = strlen("ethereum-secp256k1") + 1;
            size_t address_len = strlen(address) + 1;

            if (deleted)
            {
                child_send_response(fd, ENOENT, NULL, 0);
            }
            else if (key_type_len + address_len > sizeof(response))
            {
                child_send_response(fd, ENOMEM, NULL, 0);
            }
            else
            {
                memcpy(response, "ethereum-secp256k1", key_type_len);
                memcpy(response + key_type_len, address, address_len);
                child_send_response(fd, 0, response, key_type_len + address_len);
            }
        }
        else if (request.op == SIM_OP_SIGN)
        {
            char *signature = NULL;

            if (deleted)
            {
                child_send_response(fd, ENOENT, NULL, 0);
            }
            else
            {
                signature = oms_wallet_sign_wallet_request_preimage_bytes(
                    wallet.seckey,
                    payload,
                    request.len);
                if (!signature)
                {
                    child_send_response(fd, EIO, NULL, 0);
                }
                else
                {
                    child_send_response(fd, 0, signature, strlen(signature));
                }
                free(signature);
            }
        }
        else if (request.op == SIM_OP_EXIT)
        {
            free(payload);
            child_send_response(fd, 0, NULL, 0);
            break;
        }
        else
        {
            child_send_response(fd, EINVAL, NULL, 0);
        }

        free(payload);
    }

    free(address);
    if (initialized)
    {
        eoa_wallet_destroy(&wallet);
    }
    close(fd);
    _exit(0);
}

static int sim_create(void *ctx, char **out_signer_id)
{
    sim_provider_ctx *sim = ctx;

    if (!sim || !out_signer_id)
    {
        return EINVAL;
    }
    *out_signer_id = NULL;
    return sim_send_request(sim->fd, SIM_OP_CREATE, NULL, 0, out_signer_id);
}

static int sim_delete(void *ctx, const char *signer_id)
{
    sim_provider_ctx *sim = ctx;

    if (!sim || !signer_id)
    {
        return EINVAL;
    }
    return sim_send_request(
        sim->fd,
        SIM_OP_DELETE,
        (const uint8_t *)signer_id,
        strlen(signer_id),
        NULL);
}

static int sim_get_credential(
    void *ctx,
    const char *signer_id,
    char **out_key_type,
    char **out_credential)
{
    sim_provider_ctx *sim = ctx;
    char *payload = NULL;
    size_t key_type_len;
    int status;

    if (!sim || !signer_id || !out_key_type || !out_credential)
    {
        return EINVAL;
    }

    *out_key_type = NULL;
    *out_credential = NULL;
    status = sim_send_request(
        sim->fd,
        SIM_OP_GET_CREDENTIAL,
        (const uint8_t *)signer_id,
        strlen(signer_id),
        &payload);
    if (status != 0)
    {
        free(payload);
        return status;
    }

    key_type_len = strlen(payload) + 1;
    *out_key_type = dup_string(payload);
    *out_credential = dup_string(payload + key_type_len);
    if (!*out_key_type || !*out_credential)
    {
        free(payload);
        free(*out_key_type);
        free(*out_credential);
        *out_key_type = NULL;
        *out_credential = NULL;
        return ENOMEM;
    }

    snprintf(sim->credential, sizeof(sim->credential), "%s", *out_credential);
    free(payload);
    return 0;
}

static int sim_sign_authorization_message(
    void *ctx,
    const char *signer_id,
    const uint8_t *message,
    size_t message_len,
    char **out_signature_hex)
{
    sim_provider_ctx *sim = ctx;
    (void)signer_id;

    if (!sim || (!message && message_len != 0) || !out_signature_hex)
    {
        return EINVAL;
    }

    if (message_len >= sizeof(sim->last_message))
    {
        return EOVERFLOW;
    }
    memcpy(sim->last_message, message, message_len);
    sim->last_message[message_len] = '\0';
    sim->last_message_len = message_len;
    *out_signature_hex = NULL;
    return sim_send_request(sim->fd, SIM_OP_SIGN, message, message_len, out_signature_hex);
}

static void sim_free_string(void *ctx, char *value)
{
    (void)ctx;
    free(value);
}

static int prepare_dummy_wallet_request(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error)
{
    (void)request;
    (void)error;

    prepared_request->http_method = dup_string("POST");
    prepared_request->path = dup_string("/rpc/Wallet/CreateWallet");
    prepared_request->content_type = dup_string("application/json");
    prepared_request->body = dup_string("{\"type\":\"ethereum\"}");
    if (!prepared_request->http_method || !prepared_request->path ||
        !prepared_request->content_type || !prepared_request->body)
    {
        return -1;
    }
    prepared_request->body_len = strlen(prepared_request->body);
    return 0;
}

static int parse_dummy_wallet_response(
    const waas_http_response *http_response,
    void *response,
    waas_error *error)
{
    int *parsed = response;
    (void)error;

    if (!http_response || http_response->status_code != 200 ||
        !http_response->body || strcmp(http_response->body, "{\"ok\":true}") != 0)
    {
        return -1;
    }

    *parsed = 1;
    return 0;
}

static int capture_transport(
    void *ctx,
    const char *base_url,
    const waas_prepared_request *request,
    waas_http_response *response,
    waas_error *error)
{
    capture_transport_ctx *capture = ctx;
    const char *auth_header = NULL;

    (void)error;
    (void)base_url;

    if (!capture || !request || !response)
    {
        return EINVAL;
    }

    capture->send_count++;
    for (size_t i = 0; i < request->headers_count; ++i)
    {
        if (strncmp(request->headers[i], "Authorization: ", strlen("Authorization: ")) == 0)
        {
            auth_header = request->headers[i];
            break;
        }
    }
    if (!auth_header)
    {
        return EINVAL;
    }

    snprintf(capture->auth_header, sizeof(capture->auth_header), "%s", auth_header);
    expect_string("transport method", request->http_method, "POST");
    expect_string("transport path", request->path, "/rpc/Wallet/CreateWallet");
    expect_string("transport body", request->body, "{\"type\":\"ethereum\"}");
    expect_contains("auth key type", auth_header, "Authorization: ethereum-secp256k1 ");
    expect_contains("auth scope", auth_header, "scope=\"scope-test\"");
    expect_contains("auth credential", auth_header, capture->provider->credential);
    expect_contains("auth signature", auth_header, "sig=\"0x");

    response->status_code = 200;
    response->body = dup_string("{\"ok\":true}");
    response->body_len = strlen(response->body);
    return response->body ? 0 : ENOMEM;
}

int main(void)
{
    int fds[2];
    pid_t child;
    int child_status = 0;
    char tmp_template[] = "/tmp/oms-wallet-embedded-secure-XXXXXX";
    char *storage_dir = mkdtemp(tmp_template);
    char seckey_path[PATH_MAX];
    struct stat st;
    sim_provider_ctx sim = {0};
    capture_transport_ctx capture = {0};
    oms_wallet_auth_signer_provider_t provider = {
        OMS_WALLET_AUTH_SIGNER_PROVIDER_ABI_VERSION,
        &sim,
        sim_create,
        sim_delete,
        sim_get_credential,
        sim_sign_authorization_message,
        sim_free_string
    };
    oms_wallet_rpc_context rpc;
    oms_wallet_sdk_t sdk;
    int dummy_request = 0;
    int parsed_response = 0;
    char *signer_id = NULL;
    char *signature = NULL;
    char *recovered_address = NULL;

    if (!storage_dir)
    {
        perror("mkdtemp");
        return 1;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
    {
        perror("socketpair");
        return 1;
    }

    child = fork();
    if (child < 0)
    {
        perror("fork");
        return 1;
    }
    if (child == 0)
    {
        close(fds[0]);
        secure_element_child_main(fds[1]);
    }
    close(fds[1]);
    sim.fd = fds[0];
    capture.provider = &sim;

    if (oms_wallet_sdk_init(&sdk, "test-access-key") != 0 ||
        oms_wallet_config_set_storage_dir(&sdk, storage_dir) != 0 ||
        oms_wallet_config_set_wallet_auth_scope(&sdk, "scope-test") != 0 ||
        oms_wallet_config_set_auth_signer_provider(&sdk, &provider) != 0 ||
        oms_wallet_config_set_transport(&sdk, capture_transport, &capture) != 0)
    {
        fail("failed to initialize SDK config");
    }

    if (oms_wallet_auth_signer_create(&sdk, &signer_id) != 0)
    {
        fail("failed to create simulator signer");
    }
    sdk.auth_signer_id = signer_id;
    signer_id = NULL;

    oms_wallet_rpc_context_init(&rpc, &sdk);
    if (oms_wallet_rpc_execute(
            &rpc,
            &dummy_request,
            &parsed_response,
            prepare_dummy_wallet_request,
            parse_dummy_wallet_response) != 0)
    {
        fprintf(stderr, "rpc execute failed: %s\n",
            rpc.error.message ? rpc.error.message : "unknown error");
        return 1;
    }

    expect_int("parsed response", parsed_response, 1);
    expect_int("transport send count", capture.send_count, 1);
    expect_contains(
        "simulator signed SDK canonical message",
        sim.last_message,
        "POST /rpc/Wallet/CreateWallet\nnonce: ");
    expect_contains(
        "simulator signed SDK request body",
        sim.last_message,
        "\n\n{\"type\":\"ethereum\"}");
    signature = extract_authorization_signature(capture.auth_header);
    if (!signature)
    {
        fail("failed to extract authorization signature");
    }
    recovered_address = recover_authorization_address(
        signature,
        (const uint8_t *)sim.last_message,
        sim.last_message_len);
    expect_string("recovered authorization signer", recovered_address, sim.credential);

    snprintf(seckey_path, sizeof(seckey_path), "%s/seckey", storage_dir);
    if (stat(seckey_path, &st) == 0)
    {
        fail("default seckey storage was unexpectedly created");
    }
    if (errno != ENOENT)
    {
        perror("stat seckey");
        return 1;
    }

    oms_wallet_rpc_context_free(&rpc);
    clear_current_signer(&sdk);
    oms_wallet_sdk_cleanup(&sdk);
    free(signature);
    free(recovered_address);
    sim_send_request(sim.fd, SIM_OP_EXIT, NULL, 0, NULL);
    close(sim.fd);
    waitpid(child, &child_status, 0);
    rmdir(storage_dir);

    if (!WIFEXITED(child_status) || WEXITSTATUS(child_status) != 0)
    {
        fail("secure element simulator exited unsuccessfully");
    }

    printf("embedded_secure_auth_provider_test passed\n");
    return 0;
}
