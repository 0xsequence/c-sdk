#include "oms_wallet.h"
#include "oms_wallet_internal.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chains/chain_bindings.h"
#include "generated/waas/waas.gen.h"
#include "runtime/oms_wallet_runtime.h"
#include "storage/secure_storage.h"
#include "utils/timestamps.h"
#include "wallet/oms_wallet_config.h"
#include "wallet/oms_wallet_request_signing.h"

static const char *g_default_wallet_type = "ethereum";
static const char *g_wallet_rpc_path = "/rpc/Wallet";

const char *oms_wallet_default_wallet_type(void)
{
    return g_default_wallet_type;
}

void clear_current_signer(oms_wallet_sdk_t *sdk)
{
    if (!sdk)
    {
        return;
    }

    free(sdk->auth_signer_id);
    sdk->auth_signer_id = NULL;
}

void oms_wallet_set_waas_error(
    waas_error *error,
    const char *name,
    const char *message,
    const char *cause
)
{
    if (!error)
    {
        return;
    }

    waas_error_free(error);
    error->name = waas_strdup(name ? name : "ClientError");
    error->message = waas_strdup(message ? message : "request failed");
    error->cause = cause ? waas_strdup(cause) : NULL;
}

static int default_session_write(void *ctx, const char *key, const char *value)
{
    oms_wallet_sdk_t *sdk = (oms_wallet_sdk_t *)ctx;
    return secure_store_write_string_at(sdk ? sdk->config.storage_dir : NULL, key, value);
}

static int default_session_read(void *ctx, const char *key, char **out_value)
{
    oms_wallet_sdk_t *sdk = (oms_wallet_sdk_t *)ctx;
    return secure_store_read_string_at(sdk ? sdk->config.storage_dir : NULL, key, out_value);
}

static int default_session_delete(void *ctx, const char *key)
{
    oms_wallet_sdk_t *sdk = (oms_wallet_sdk_t *)ctx;
    return secure_store_delete_at(sdk ? sdk->config.storage_dir : NULL, key);
}

static void default_session_free_string(void *ctx, char *value)
{
    (void)ctx;
    free(value);
}

static int default_session_status_is_not_found(void *ctx, int status)
{
    (void)ctx;
    return secure_store_status_is_not_found(status);
}

static const oms_wallet_session_store_provider_t default_session_store_provider = {
    NULL,
    default_session_write,
    default_session_read,
    default_session_delete,
    default_session_free_string,
    default_session_status_is_not_found
};

static const oms_wallet_session_store_provider_t *oms_wallet_get_session_store_provider(oms_wallet_sdk_t *sdk)
{
    if (!sdk)
    {
        return NULL;
    }

    if (sdk->config.has_session_store_provider)
    {
        return &sdk->config.session_store_provider;
    }

    return &default_session_store_provider;
}

static void *oms_wallet_session_store_ctx(
    oms_wallet_sdk_t *sdk,
    const oms_wallet_session_store_provider_t *provider)
{
    if (provider == &default_session_store_provider)
    {
        return sdk;
    }
    return provider ? provider->ctx : NULL;
}

int oms_wallet_session_write_string(oms_wallet_sdk_t *sdk, const char *key, const char *value)
{
    const oms_wallet_session_store_provider_t *provider = oms_wallet_get_session_store_provider(sdk);

    if (!provider)
    {
        return EINVAL;
    }

    return provider->write_string(oms_wallet_session_store_ctx(sdk, provider), key, value);
}

int oms_wallet_session_read_string(oms_wallet_sdk_t *sdk, const char *key, char **out_value)
{
    const oms_wallet_session_store_provider_t *provider = oms_wallet_get_session_store_provider(sdk);

    if (!provider)
    {
        return EINVAL;
    }

    return provider->read_string(oms_wallet_session_store_ctx(sdk, provider), key, out_value);
}

int oms_wallet_session_delete(oms_wallet_sdk_t *sdk, const char *key)
{
    const oms_wallet_session_store_provider_t *provider = oms_wallet_get_session_store_provider(sdk);

    if (!provider)
    {
        return EINVAL;
    }

    return provider->delete_key(oms_wallet_session_store_ctx(sdk, provider), key);
}

void oms_wallet_session_free_string(oms_wallet_sdk_t *sdk, char *value)
{
    const oms_wallet_session_store_provider_t *provider = oms_wallet_get_session_store_provider(sdk);

    if (!value)
    {
        return;
    }

    if (provider && provider->free_string)
    {
        provider->free_string(oms_wallet_session_store_ctx(sdk, provider), value);
    }
    else
    {
        free(value);
    }
}

int oms_wallet_session_status_is_not_found(oms_wallet_sdk_t *sdk, int status)
{
    const oms_wallet_session_store_provider_t *provider = oms_wallet_get_session_store_provider(sdk);

    if (provider && provider->is_not_found)
    {
        return provider->is_not_found(oms_wallet_session_store_ctx(sdk, provider), status);
    }

    return status == ENOENT;
}

static char *oms_wallet_build_access_key_header(oms_wallet_sdk_t *sdk)
{
    char *header;
    size_t len;

    if (!sdk || !sdk->config.access_key || sdk->config.access_key[0] == '\0')
    {
        return NULL;
    }

    len = strlen("X-Access-Key: ") + strlen(sdk->config.access_key) + 1;
    header = malloc(len);
    if (!header)
    {
        return NULL;
    }

    snprintf(header, len, "X-Access-Key: %s", sdk->config.access_key);
    return header;
}

static char *oms_wallet_client_base_url(oms_wallet_sdk_t *sdk)
{
    char *base_url;
    size_t url_len;
    size_t suffix_len;
    const char *wallet_rpc_url = sdk ? sdk->config.wallet_rpc_url : NULL;

    if (!wallet_rpc_url || wallet_rpc_url[0] == '\0')
    {
        return NULL;
    }

    url_len = strlen(wallet_rpc_url);
    suffix_len = strlen(g_wallet_rpc_path);
    if (url_len >= suffix_len &&
        strcmp(wallet_rpc_url + url_len - suffix_len, g_wallet_rpc_path) == 0)
    {
        size_t base_len = url_len - suffix_len;

        base_url = malloc(base_len + 1);
        if (!base_url)
        {
            return NULL;
        }

        memcpy(base_url, wallet_rpc_url, base_len);
        base_url[base_len] = '\0';
        return base_url;
    }

    return waas_strdup(wallet_rpc_url);
}

static const char *oms_wallet_request_endpoint(
    const waas_prepared_request *prepared_request,
    waas_error *error
)
{
    size_t prefix_len;

    if (!prepared_request || !prepared_request->path)
    {
        oms_wallet_set_waas_error(
            error,
            "ClientError",
            "prepared request path must be available",
            NULL);
        return NULL;
    }

    prefix_len = strlen(g_wallet_rpc_path);
    if (strncmp(prepared_request->path, g_wallet_rpc_path, prefix_len) != 0)
    {
        oms_wallet_set_waas_error(
            error,
            "ClientError",
            "prepared request path does not target Wallet RPC",
            prepared_request->path);
        return NULL;
    }

    return prepared_request->path + prefix_len;
}

int oms_wallet_parse_wallet_type(
    const char *wallet_type,
    waas_wallet_type *out,
    waas_error *error
)
{
    if (!wallet_type || !out || waas_wallet_type_from_string(wallet_type, out) != 0)
    {
        oms_wallet_set_waas_error(
            error,
            "ClientError",
            "unsupported wallet type",
            wallet_type);
        return -1;
    }

    return 0;
}

void oms_wallet_log_waas_error(const char *operation, const waas_error *error)
{
    if (!error || !error->message)
    {
        return;
    }

    fprintf(
        stderr,
        "%s failed: %s%s%s%s\n",
        operation ? operation : "WAAS call",
        error->message,
        error->cause ? " (" : "",
        error->cause ? error->cause : "",
        error->cause ? ")" : "");
}

static waas_wallet_client *oms_wallet_client_create(oms_wallet_sdk_t *sdk, waas_error *error)
{
    char *access_key_header = NULL;
    char *base_url = NULL;
    const char *headers[3];
    size_t headers_count = 0;
    waas_client_options options;
    waas_wallet_client *client = NULL;
    int runtime_acquired = 0;

    if (oms_wallet_runtime_acquire(error) != 0)
    {
        return NULL;
    }
    runtime_acquired = 1;

    access_key_header = oms_wallet_build_access_key_header(sdk);
    base_url = oms_wallet_client_base_url(sdk);

    if (!base_url)
    {
        oms_wallet_set_waas_error(
            error,
            "ClientError",
            "failed to prepare generated wallet client",
            NULL);
        goto cleanup;
    }

    waas_client_options_init(&options);
    options.max_response_bytes = sdk ? sdk->config.max_response_bytes : 0;
    if (access_key_header)
    {
        headers[headers_count++] = access_key_header;
    }
    if (sdk && sdk->config.origin_header && sdk->config.origin_header[0] != '\0')
    {
        headers[headers_count++] = sdk->config.origin_header;
    }
    headers[headers_count++] = "Accept: application/json";
    options.headers = headers;
    options.headers_count = headers_count;

    client = waas_wallet_client_create(base_url, &options);
    if (!client)
    {
        oms_wallet_set_waas_error(
            error,
            "ClientError",
            "failed to create generated wallet client",
            NULL);
    }

cleanup:
    if (!client && runtime_acquired)
    {
        oms_wallet_runtime_release();
    }
    free(base_url);
    free(access_key_header);
    return client;
}

int oms_wallet_authorize_prepared_request(
    oms_wallet_sdk_t *sdk,
    waas_prepared_request *prepared_request,
    waas_error *error
)
{
    long long nonce_int;
    char nonce[32];
    char *key_type = NULL;
    char *credential = NULL;
    char metadata[64];
    char *message = NULL;
    size_t message_len = 0;
    char *signature = NULL;
    char *auth_header = NULL;
    int rc = -1;

    if (!sdk || !prepared_request || !sdk->auth_signer_id)
    {
        oms_wallet_set_waas_error(
            error,
            "ClientError",
            "prepared request and signer must be available",
            NULL);
        return -1;
    }

    if (!oms_wallet_request_endpoint(prepared_request, error) || !prepared_request->body)
    {
        if (error && !error->message)
        {
            oms_wallet_set_waas_error(
                error,
                "ClientError",
                "prepared request body must be available",
                NULL);
        }
        goto cleanup;
    }

    nonce_int = timestamp_next_nonce();
    snprintf(nonce, sizeof(nonce), "%lld", nonce_int);
    snprintf(metadata, sizeof(metadata), "nonce: %s", nonce);

    message = oms_wallet_build_authorization_message(
        prepared_request->http_method,
        prepared_request->path,
        metadata,
        (const uint8_t *)prepared_request->body,
        prepared_request->body_len,
        &message_len);
    if (!message)
    {
        oms_wallet_set_waas_error(
            error,
            "ClientError",
            "failed to build authorization message",
            NULL);
        goto cleanup;
    }

    if (oms_wallet_auth_signer_get_credential(
            sdk,
            sdk->auth_signer_id,
            &key_type,
            &credential) != 0 ||
        oms_wallet_auth_signer_sign_authorization_message(
            sdk,
            sdk->auth_signer_id,
            (const uint8_t *)message,
            message_len,
            &signature) != 0)
    {
        oms_wallet_set_waas_error(
            error,
            "ClientError",
            "auth signer failed to prepare signed wallet request",
            NULL);
        goto cleanup;
    }

    if (!key_type || !credential || !signature)
    {
        oms_wallet_set_waas_error(
            error,
            "ClientError",
            "auth signer returned incomplete signed wallet request",
            NULL);
        goto cleanup;
    }

    auth_header =
        oms_wallet_build_wallet_authorization_header(
            key_type,
            sdk->config.wallet_auth_scope ? sdk->config.wallet_auth_scope : "",
            credential,
            nonce,
            signature);

    if (!auth_header)
    {
        oms_wallet_set_waas_error(
            error,
            "ClientError",
            "failed to prepare signed wallet request",
            NULL);
        goto cleanup;
    }

    if (waas_prepared_request_add_header(prepared_request, auth_header) != 0)
    {
        oms_wallet_set_waas_error(
            error,
            "ClientError",
            "failed to append Authorization header to prepared request",
            NULL);
        goto cleanup;
    }

    rc = 0;

cleanup:
    free(auth_header);
    if (message)
    {
        memset(message, 0, message_len);
    }
    free(message);
    oms_wallet_auth_signer_free_string(sdk, signature);
    oms_wallet_auth_signer_free_string(sdk, credential);
    oms_wallet_auth_signer_free_string(sdk, key_type);
    return rc;
}

static int oms_wallet_send_authorized_request(
    oms_wallet_sdk_t *sdk,
    waas_wallet_client *client,
    waas_prepared_request *prepared_request,
    waas_http_response *http_response,
    waas_error *error
)
{
    char *base_url = NULL;
    int rc;

    if (!sdk || !prepared_request || !http_response)
    {
        oms_wallet_set_waas_error(
            error,
            "ClientError",
            "SDK, prepared request, and response must be available",
            NULL);
        return -1;
    }

    if (oms_wallet_authorize_prepared_request(sdk, prepared_request, error) != 0)
    {
        return -1;
    }

    if (sdk->config.has_transport)
    {
        base_url = oms_wallet_client_base_url(sdk);
        if (!base_url)
        {
            oms_wallet_set_waas_error(
                error,
                "ClientError",
                "failed to prepare wallet transport base URL",
                NULL);
            return -1;
        }

        rc = sdk->config.transport(
            sdk->config.transport_ctx,
            base_url,
            prepared_request,
            http_response,
            error);
        free(base_url);
        return rc;
    }

    if (!client)
    {
        oms_wallet_set_waas_error(
            error,
            "ClientError",
            "generated wallet client must be available",
            NULL);
        return -1;
    }

    return waas_wallet_client_send_prepared_request(
        client,
        prepared_request,
        http_response,
        error);
}

void oms_wallet_rpc_context_init(oms_wallet_rpc_context *rpc, oms_wallet_sdk_t *sdk)
{
    if (!rpc)
    {
        return;
    }

    memset(rpc, 0, sizeof(*rpc));
    rpc->sdk = sdk;
    waas_prepared_request_init(&rpc->prepared_request);
    waas_http_response_init(&rpc->http_response);
    waas_error_init(&rpc->error);
}

void oms_wallet_rpc_context_free(oms_wallet_rpc_context *rpc)
{
    if (!rpc)
    {
        return;
    }

    if (rpc->client)
    {
        waas_wallet_client_destroy(rpc->client);
        rpc->client = NULL;
        oms_wallet_runtime_release();
    }
    waas_http_response_free(&rpc->http_response);
    waas_prepared_request_free(&rpc->prepared_request);
    waas_error_free(&rpc->error);
}

int oms_wallet_rpc_execute(
    oms_wallet_rpc_context *rpc,
    const void *request,
    void *response,
    oms_wallet_prepare_request_fn prepare_request,
    oms_wallet_parse_response_fn parse_response
)
{
    if (!rpc || !rpc->sdk || !request || !response || !prepare_request || !parse_response)
    {
        return -1;
    }

    if (!rpc->sdk->config.has_transport)
    {
        rpc->client = oms_wallet_client_create(rpc->sdk, &rpc->error);
        if (!rpc->client)
        {
            return -1;
        }
    }

    if (prepare_request(request, &rpc->prepared_request, &rpc->error) != 0)
    {
        return -1;
    }

    if (oms_wallet_send_authorized_request(
            rpc->sdk,
            rpc->client,
            &rpc->prepared_request,
            &rpc->http_response,
            &rpc->error) != 0)
    {
        return -1;
    }

    return parse_response(&rpc->http_response, response, &rpc->error);
}

#define OMS_WALLET_DEFINE_WALLET_RPC_ADAPTERS(name, request_type, response_type)           \
    int oms_wallet_##name##_prepare_request(                                               \
        const void *request,                                                               \
        waas_prepared_request *prepared_request,                                           \
        waas_error *error)                                                                 \
    {                                                                                      \
        return waas_wallet_##name##_prepare_request(                                       \
            (const request_type *)request,                                                 \
            prepared_request,                                                              \
            error);                                                                        \
    }                                                                                      \
                                                                                           \
    int oms_wallet_##name##_parse_response(                                                \
        const waas_http_response *http_response,                                           \
        void *response,                                                                    \
        waas_error *error)                                                                 \
    {                                                                                      \
        return waas_wallet_##name##_parse_response(                                        \
            http_response,                                                                 \
            (response_type *)response,                                                     \
            error);                                                                        \
    }

OMS_WALLET_DEFINE_WALLET_RPC_ADAPTERS(
    commit_verifier,
    waas_wallet_commit_verifier_request,
    waas_wallet_commit_verifier_response)
OMS_WALLET_DEFINE_WALLET_RPC_ADAPTERS(
    complete_auth,
    waas_wallet_complete_auth_request,
    waas_wallet_complete_auth_response)
OMS_WALLET_DEFINE_WALLET_RPC_ADAPTERS(
    create_wallet,
    waas_wallet_create_wallet_request,
    waas_wallet_create_wallet_response)
OMS_WALLET_DEFINE_WALLET_RPC_ADAPTERS(
    use_wallet,
    waas_wallet_use_wallet_request,
    waas_wallet_use_wallet_response)
OMS_WALLET_DEFINE_WALLET_RPC_ADAPTERS(
    sign_message,
    waas_wallet_sign_message_request,
    waas_wallet_sign_message_response)
OMS_WALLET_DEFINE_WALLET_RPC_ADAPTERS(
    prepare_ethereum_transaction,
    waas_wallet_prepare_ethereum_transaction_request,
    waas_wallet_prepare_ethereum_transaction_response)
OMS_WALLET_DEFINE_WALLET_RPC_ADAPTERS(
    execute,
    waas_wallet_execute_request,
    waas_wallet_execute_response)
OMS_WALLET_DEFINE_WALLET_RPC_ADAPTERS(
    get_transaction_status,
    waas_wallet_get_transaction_status_request,
    waas_wallet_get_transaction_status_response)

#undef OMS_WALLET_DEFINE_WALLET_RPC_ADAPTERS

static waas_wallet *oms_wallet_take_wallet(waas_wallet **wallet)
{
    waas_wallet *value;

    if (!wallet)
    {
        return NULL;
    }

    value = *wallet;
    *wallet = NULL;
    return value;
}

int oms_wallet_require_signer_initialized(oms_wallet_sdk_t *sdk)
{
    if (!sdk || !sdk->auth_signer_id || sdk->auth_signer_id[0] == '\0')
    {
        fprintf(stderr, "No signer initialized\n");
        return 0;
    }

    return 1;
}

int oms_wallet_finalize_wallet_response(
    oms_wallet_rpc_context *rpc,
    waas_wallet **wallet_out,
    waas_wallet **response_wallet,
    const char *operation
)
{
    if (!rpc || !rpc->sdk || !wallet_out || !response_wallet)
    {
        return -1;
    }

    *wallet_out = oms_wallet_take_wallet(response_wallet);
    if (!*wallet_out || !(*wallet_out)->id || !(*wallet_out)->address)
    {
        oms_wallet_set_waas_error(
            &rpc->error,
            "ClientError",
            operation ? operation : "wallet response missing id or address",
            NULL);
        return -1;
    }

    free(rpc->sdk->wallet_id);
    rpc->sdk->wallet_id = waas_strdup((*wallet_out)->id);
    if (!rpc->sdk->wallet_id)
    {
        oms_wallet_set_waas_error(
            &rpc->error,
            "ClientError",
            "failed to persist wallet id in memory",
            NULL);
        return -1;
    }

    free(rpc->sdk->challenge);
    rpc->sdk->challenge = NULL;
    free(rpc->sdk->verifier);
    rpc->sdk->verifier = NULL;

    oms_wallet_session_write_string(rpc->sdk, "oms_wallet_id", (*wallet_out)->id);
    oms_wallet_session_write_string(rpc->sdk, "oms_wallet_address", (*wallet_out)->address);
    return 0;
}

int oms_wallet_prepare_wallet_target_params(
    oms_wallet_sdk_t *sdk,
    const char *chain_id,
    char **network_field,
    char **wallet_id_field,
    waas_error *error,
    const char *operation
)
{
    char *wallet_id = NULL;
    const char *network = NULL;

    if (!sdk || !network_field || !wallet_id_field)
    {
        return -1;
    }

    if (sdk->wallet_id && sdk->wallet_id[0] != '\0')
    {
        wallet_id = waas_strdup(sdk->wallet_id);
    }
    else
    {
        oms_wallet_session_read_string(sdk, "oms_wallet_id", &wallet_id);
    }

    network = (chain_id && chain_id[0] != '\0') ? chain_id : NULL;
    *network_field = network ? waas_strdup(network) : NULL;
    *wallet_id_field = wallet_id;

    if ((network && !*network_field) ||
        !wallet_id || !network)
    {
        oms_wallet_set_waas_error(
            error,
            "ClientError",
            operation ? operation : "failed to prepare wallet target params",
            NULL);
        free(wallet_id);
        *wallet_id_field = NULL;
        return -1;
    }

    return 0;
}
