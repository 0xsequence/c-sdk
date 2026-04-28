#ifndef OMS_WALLET_CONFIG_H
#define OMS_WALLET_CONFIG_H

#include "generated/waas/waas.gen.h"
#include "wallet/oms_wallet_auth_signer.h"

typedef struct oms_wallet_sdk oms_wallet_sdk_t;

typedef int (*oms_wallet_transport_fn)(
    void *ctx,
    const char *base_url,
    const waas_prepared_request *request,
    waas_http_response *response,
    waas_error *error);

typedef struct oms_wallet_session_store_provider {
    void *ctx;
    int (*write_string)(void *ctx, const char *key, const char *value);
    int (*read_string)(void *ctx, const char *key, char **out_value);
    int (*delete_key)(void *ctx, const char *key);
    void (*free_string)(void *ctx, char *value);
    int (*is_not_found)(void *ctx, int status);
} oms_wallet_session_store_provider_t;

typedef struct {
    char *access_key;
    char *indexer_url_template;
    char *api_rpc_url;
    char *wallet_rpc_url;
    char *wallet_auth_scope;
    char *origin_header;
    char *storage_dir;
    int has_auth_signer_provider;
    oms_wallet_auth_signer_provider_t auth_signer_provider;
    int has_transport;
    oms_wallet_transport_fn transport;
    void *transport_ctx;
    int has_session_store_provider;
    oms_wallet_session_store_provider_t session_store_provider;
    size_t max_response_bytes;
} oms_wallet_config_t;

struct oms_wallet_sdk {
    oms_wallet_config_t config;
    char *auth_signer_id;
    char *challenge;
    char *verifier;
    char *wallet_id;
};

int oms_wallet_sdk_init(oms_wallet_sdk_t *sdk, const char *access_key);
void oms_wallet_sdk_cleanup(oms_wallet_sdk_t *sdk);

int oms_wallet_config_set_access_key(oms_wallet_sdk_t *sdk, const char *access_key);
int oms_wallet_config_set_indexer_url_template(oms_wallet_sdk_t *sdk, const char *indexer_url_template);
int oms_wallet_config_set_api_rpc_url(oms_wallet_sdk_t *sdk, const char *api_rpc_url);
int oms_wallet_config_set_wallet_rpc_url(oms_wallet_sdk_t *sdk, const char *wallet_rpc_url);
int oms_wallet_config_set_wallet_auth_scope(oms_wallet_sdk_t *sdk, const char *wallet_auth_scope);
int oms_wallet_config_set_origin_header(oms_wallet_sdk_t *sdk, const char *origin_header);
int oms_wallet_config_set_storage_dir(oms_wallet_sdk_t *sdk, const char *storage_dir);
int oms_wallet_config_set_auth_signer_provider(
    oms_wallet_sdk_t *sdk,
    const oms_wallet_auth_signer_provider_t *provider);
int oms_wallet_config_set_session_store_provider(
    oms_wallet_sdk_t *sdk,
    const oms_wallet_session_store_provider_t *provider);
int oms_wallet_config_set_max_response_bytes(oms_wallet_sdk_t *sdk, size_t max_response_bytes);

/*
 * Overrides all SDK network transport. Wallet RPC requests are passed after the
 * SDK has appended Authorization.
 */
int oms_wallet_config_set_transport(
    oms_wallet_sdk_t *sdk,
    oms_wallet_transport_fn transport,
    void *ctx);

#endif
