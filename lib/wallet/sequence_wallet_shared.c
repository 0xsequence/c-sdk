#include "sequence_connector.h"
#include "sequence_wallet_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chains/chain_bindings.h"
#include "generated/waas/waas.gen.h"
#include "storage/secure_storage.h"
#include "utils/globals.h"
#include "utils/timestamps.h"
#include "wallet/sequence_config.h"
#include "wallet/sequence_request_signing.h"

eoa_wallet_t *cur_signer = NULL;
char *cur_challenge = NULL;
char *cur_verifier = NULL;
static const char *g_default_wallet_type = "Ethereum_EOA";
static const char *g_wallet_rpc_path = "/rpc/Wallet";

const char *sequence_default_wallet_type(void)
{
    return g_default_wallet_type;
}

void clear_current_signer(void)
{
    if (!cur_signer)
    {
        return;
    }

    if (cur_signer->ctx)
    {
        eoa_wallet_destroy(cur_signer);
    }

    free(cur_signer);
    cur_signer = NULL;
}

void sequence_set_waas_error(
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

static char *sequence_build_access_key_header(void)
{
    char *header;
    size_t len;

    if (!sequence_config.access_key || sequence_config.access_key[0] == '\0')
    {
        return NULL;
    }

    len = strlen("X-Access-Key: ") + strlen(sequence_config.access_key) + 1;
    header = malloc(len);
    if (!header)
    {
        return NULL;
    }

    snprintf(header, len, "X-Access-Key: %s", sequence_config.access_key);
    return header;
}

static char *sequence_wallet_client_base_url(void)
{
    char *base_url;
    size_t url_len;
    size_t suffix_len;

    if (!g_wallet_api_url)
    {
        return NULL;
    }

    url_len = strlen(g_wallet_api_url);
    suffix_len = strlen(g_wallet_rpc_path);
    if (url_len >= suffix_len &&
        strcmp(g_wallet_api_url + url_len - suffix_len, g_wallet_rpc_path) == 0)
    {
        size_t base_len = url_len - suffix_len;

        base_url = malloc(base_len + 1);
        if (!base_url)
        {
            return NULL;
        }

        memcpy(base_url, g_wallet_api_url, base_len);
        base_url[base_len] = '\0';
        return base_url;
    }

    return waas_strdup(g_wallet_api_url);
}

static const char *sequence_wallet_request_endpoint(
    const waas_prepared_request *prepared_request,
    waas_error *error
)
{
    size_t prefix_len;

    if (!prepared_request || !prepared_request->path)
    {
        sequence_set_waas_error(
            error,
            "ClientError",
            "prepared request path must be available",
            NULL);
        return NULL;
    }

    prefix_len = strlen(g_wallet_rpc_path);
    if (strncmp(prepared_request->path, g_wallet_rpc_path, prefix_len) != 0)
    {
        sequence_set_waas_error(
            error,
            "ClientError",
            "prepared request path does not target Wallet RPC",
            prepared_request->path);
        return NULL;
    }

    return prepared_request->path + prefix_len;
}

int sequence_parse_wallet_type(
    const char *wallet_type,
    waas_wallet_type *out,
    waas_error *error
)
{
    if (!wallet_type || !out || waas_wallet_type_from_string(wallet_type, out) != 0)
    {
        sequence_set_waas_error(
            error,
            "ClientError",
            "unsupported wallet type",
            wallet_type);
        return -1;
    }

    return 0;
}

void sequence_log_waas_error(const char *operation, const waas_error *error)
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

static waas_wallet_client *sequence_wallet_client_create(waas_error *error)
{
    char *access_key_header = NULL;
    char *base_url = NULL;
    const char *headers[3];
    size_t headers_count = 0;
    waas_client_options options;
    waas_wallet_client *client = NULL;

    access_key_header = sequence_build_access_key_header();
    base_url = sequence_wallet_client_base_url();

    if (!base_url)
    {
        sequence_set_waas_error(
            error,
            "ClientError",
            "failed to prepare generated wallet client",
            NULL);
        goto cleanup;
    }

    waas_client_options_init(&options);
    if (access_key_header)
    {
        headers[headers_count++] = access_key_header;
    }
    headers[headers_count++] = "Origin: http://localhost:3000";
    headers[headers_count++] = "Accept: application/json";
    options.headers = headers;
    options.headers_count = headers_count;

    client = waas_wallet_client_create(base_url, &options);
    if (!client)
    {
        sequence_set_waas_error(
            error,
            "ClientError",
            "failed to create generated wallet client",
            NULL);
    }

cleanup:
    free(base_url);
    free(access_key_header);
    return client;
}

static int sequence_wallet_send_authorized_request(
    waas_wallet_client *client,
    waas_prepared_request *prepared_request,
    waas_http_response *http_response,
    waas_error *error
)
{
    const char *endpoint;
    long long nonce_int;
    char nonce[32];
    char *address = NULL;
    char *data_to_sign = NULL;
    char *signature = NULL;
    char *auth_header = NULL;
    int rc = -1;

    if (!client || !prepared_request || !http_response || !cur_signer)
    {
        sequence_set_waas_error(
            error,
            "ClientError",
            "client, prepared request, response, and signer must be available",
            NULL);
        return -1;
    }

    endpoint = sequence_wallet_request_endpoint(prepared_request, error);
    if (!endpoint || !prepared_request->body)
    {
        if (!error->message)
        {
            sequence_set_waas_error(
                error,
                "ClientError",
                "prepared request body must be available",
                NULL);
        }
        goto cleanup;
    }

    nonce_int = timestamp_next_nonce();
    snprintf(nonce, sizeof(nonce), "%lld", nonce_int);

    address = sequence_wallet_address_from_seckey(cur_signer->seckey);
    data_to_sign =
        sequence_build_wallet_request_preimage(endpoint, nonce, prepared_request->body);
    signature = sequence_sign_wallet_request_preimage(cur_signer->seckey, data_to_sign);
    auth_header =
        sequence_build_wallet_authorization_header("@1:test", address, nonce, signature);

    if (!address || !data_to_sign || !signature || !auth_header)
    {
        sequence_set_waas_error(
            error,
            "ClientError",
            "failed to prepare signed wallet request",
            NULL);
        goto cleanup;
    }

    if (waas_prepared_request_add_header(prepared_request, auth_header) != 0)
    {
        sequence_set_waas_error(
            error,
            "ClientError",
            "failed to append Authorization header to prepared request",
            NULL);
        goto cleanup;
    }

    printf(">> Request\n%s\n%s\n\n", prepared_request->body, auth_header);

    if (waas_wallet_client_send_prepared_request(
            client,
            prepared_request,
            http_response,
            error) != 0)
    {
        goto cleanup;
    }

    rc = 0;

cleanup:
    free(auth_header);
    free(signature);
    free(data_to_sign);
    free(address);
    return rc;
}

void sequence_wallet_rpc_context_init(sequence_wallet_rpc_context *rpc)
{
    if (!rpc)
    {
        return;
    }

    memset(rpc, 0, sizeof(*rpc));
    waas_prepared_request_init(&rpc->prepared_request);
    waas_http_response_init(&rpc->http_response);
    waas_error_init(&rpc->error);
}

void sequence_wallet_rpc_context_free(sequence_wallet_rpc_context *rpc)
{
    if (!rpc)
    {
        return;
    }

    waas_wallet_client_destroy(rpc->client);
    waas_http_response_free(&rpc->http_response);
    waas_prepared_request_free(&rpc->prepared_request);
    waas_error_free(&rpc->error);
}

int sequence_wallet_rpc_execute(
    sequence_wallet_rpc_context *rpc,
    const void *request,
    void *response,
    sequence_prepare_request_fn prepare_request,
    sequence_parse_response_fn parse_response
)
{
    if (!rpc || !request || !response || !prepare_request || !parse_response)
    {
        return -1;
    }

    rpc->client = sequence_wallet_client_create(&rpc->error);
    if (!rpc->client)
    {
        return -1;
    }

    if (prepare_request(request, &rpc->prepared_request, &rpc->error) != 0)
    {
        return -1;
    }

    if (sequence_wallet_send_authorized_request(
            rpc->client,
            &rpc->prepared_request,
            &rpc->http_response,
            &rpc->error) != 0)
    {
        return -1;
    }

    return parse_response(&rpc->http_response, response, &rpc->error);
}

#define SEQUENCE_DEFINE_WALLET_RPC_ADAPTERS(name, request_type, response_type)           \
    int sequence_##name##_prepare_request(                                               \
        const void *request,                                                             \
        waas_prepared_request *prepared_request,                                         \
        waas_error *error)                                                               \
    {                                                                                    \
        return waas_wallet_##name##_prepare_request(                                     \
            (const request_type *)request,                                               \
            prepared_request,                                                            \
            error);                                                                      \
    }                                                                                    \
                                                                                         \
    int sequence_##name##_parse_response(                                                \
        const waas_http_response *http_response,                                         \
        void *response,                                                                  \
        waas_error *error)                                                               \
    {                                                                                    \
        return waas_wallet_##name##_parse_response(                                      \
            http_response,                                                               \
            (response_type *)response,                                                   \
            error);                                                                      \
    }

SEQUENCE_DEFINE_WALLET_RPC_ADAPTERS(
    commit_verifier,
    waas_commit_verifier_request,
    waas_commit_verifier_response)
SEQUENCE_DEFINE_WALLET_RPC_ADAPTERS(
    complete_auth,
    waas_complete_auth_request,
    waas_complete_auth_response)
SEQUENCE_DEFINE_WALLET_RPC_ADAPTERS(
    create_wallet,
    waas_create_wallet_request,
    waas_create_wallet_response)
SEQUENCE_DEFINE_WALLET_RPC_ADAPTERS(
    use_wallet,
    waas_use_wallet_request,
    waas_use_wallet_response)
SEQUENCE_DEFINE_WALLET_RPC_ADAPTERS(
    sign_message,
    waas_sign_message_request,
    waas_sign_message_response)
SEQUENCE_DEFINE_WALLET_RPC_ADAPTERS(
    send_transaction,
    waas_send_transaction_request,
    waas_send_transaction_response)

#undef SEQUENCE_DEFINE_WALLET_RPC_ADAPTERS

static waas_wallet *sequence_take_wallet(waas_wallet **wallet)
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

int sequence_require_signer_initialized(void)
{
    if (!cur_signer || !cur_signer->ctx)
    {
        fprintf(stderr, "No signer initialized\n");
        return 0;
    }

    return 1;
}

int sequence_finalize_wallet_response(
    sequence_wallet_rpc_context *rpc,
    waas_wallet **wallet_out,
    waas_wallet **response_wallet,
    const char *operation
)
{
    if (!rpc || !wallet_out || !response_wallet)
    {
        return -1;
    }

    *wallet_out = sequence_take_wallet(response_wallet);
    if (!*wallet_out || !(*wallet_out)->address)
    {
        sequence_set_waas_error(
            &rpc->error,
            "ClientError",
            operation ? operation : "wallet response missing address",
            NULL);
        return -1;
    }

    secure_store_write_string("sequence_wallet_address", (*wallet_out)->address);
    return 0;
}

int sequence_prepare_wallet_target_params(
    const char *chain_id,
    char **address_out,
    const char **network_out,
    char **network_field,
    char **wallet_field,
    waas_error *error,
    const char *operation
)
{
    if (!address_out || !network_out || !network_field || !wallet_field)
    {
        return -1;
    }

    secure_store_read_string("sequence_wallet_address", address_out);
    *network_out = sequence_get_chain_name(chain_id);
    *network_field = waas_strdup(*network_out);
    *wallet_field = waas_strdup(*address_out);

    if ((*network_out && !*network_field) ||
        (*address_out && !*wallet_field) ||
        !*address_out || !*network_out)
    {
        sequence_set_waas_error(
            error,
            "ClientError",
            operation ? operation : "failed to prepare wallet target params",
            NULL);
        return -1;
    }

    return 0;
}
