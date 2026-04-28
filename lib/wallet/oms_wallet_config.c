#include "oms_wallet_config.h"

#include <stdlib.h>
#include <string.h>

#define OMS_WALLET_DEFAULT_API_RPC_URL "https://api.sequence.app/rpc/API"
#define OMS_WALLET_DEFAULT_WALLET_RPC_URL "https://d1sctl7y41hot5.cloudfront.net/rpc/Wallet"
#define OMS_WALLET_DEFAULT_WALLET_AUTH_SCOPE "proj_1"
#define OMS_WALLET_DEFAULT_ORIGIN_HEADER "Origin: http://localhost:3000"
#define OMS_WALLET_DEFAULT_MAX_RESPONSE_BYTES (1024u * 1024u)

static int oms_wallet_config_replace(char **dst, const char *src)
{
    char *copy = NULL;

    if (!dst)
    {
        return -1;
    }

    if (src)
    {
        size_t len = strlen(src) + 1;

        copy = malloc(len);
        if (!copy)
        {
            return -1;
        }
        memcpy(copy, src, len);
    }

    free(*dst);
    *dst = copy;
    return 0;
}

void oms_wallet_sdk_cleanup(oms_wallet_sdk_t *sdk)
{
    if (!sdk)
    {
        return;
    }

    free(sdk->config.access_key);
    free(sdk->config.indexer_url_template);
    free(sdk->config.api_rpc_url);
    free(sdk->config.wallet_rpc_url);
    free(sdk->config.wallet_auth_scope);
    free(sdk->config.origin_header);
    free(sdk->config.storage_dir);
    free(sdk->auth_signer_id);
    free(sdk->challenge);
    free(sdk->verifier);
    free(sdk->wallet_id);
    memset(sdk, 0, sizeof(*sdk));
}

int oms_wallet_sdk_init(oms_wallet_sdk_t *sdk, const char *access_key)
{
    if (!sdk)
    {
        return -1;
    }

    memset(sdk, 0, sizeof(*sdk));
    sdk->config.max_response_bytes = OMS_WALLET_DEFAULT_MAX_RESPONSE_BYTES;

    if (oms_wallet_config_set_access_key(sdk, access_key) != 0 ||
        oms_wallet_config_set_api_rpc_url(sdk, OMS_WALLET_DEFAULT_API_RPC_URL) != 0 ||
        oms_wallet_config_set_wallet_rpc_url(sdk, OMS_WALLET_DEFAULT_WALLET_RPC_URL) != 0 ||
        oms_wallet_config_set_wallet_auth_scope(sdk, OMS_WALLET_DEFAULT_WALLET_AUTH_SCOPE) != 0 ||
        oms_wallet_config_set_origin_header(sdk, OMS_WALLET_DEFAULT_ORIGIN_HEADER) != 0)
    {
        oms_wallet_sdk_cleanup(sdk);
        return -1;
    }

    return 0;
}

int oms_wallet_config_set_access_key(oms_wallet_sdk_t *sdk, const char *access_key)
{
    return sdk ? oms_wallet_config_replace(&sdk->config.access_key, access_key) : -1;
}

int oms_wallet_config_set_indexer_url_template(oms_wallet_sdk_t *sdk, const char *indexer_url_template)
{
    return sdk ? oms_wallet_config_replace(&sdk->config.indexer_url_template, indexer_url_template) : -1;
}

int oms_wallet_config_set_api_rpc_url(oms_wallet_sdk_t *sdk, const char *api_rpc_url)
{
    return sdk ? oms_wallet_config_replace(&sdk->config.api_rpc_url, api_rpc_url) : -1;
}

int oms_wallet_config_set_wallet_rpc_url(oms_wallet_sdk_t *sdk, const char *wallet_rpc_url)
{
    return sdk ? oms_wallet_config_replace(&sdk->config.wallet_rpc_url, wallet_rpc_url) : -1;
}

int oms_wallet_config_set_wallet_auth_scope(oms_wallet_sdk_t *sdk, const char *wallet_auth_scope)
{
    return sdk ? oms_wallet_config_replace(&sdk->config.wallet_auth_scope, wallet_auth_scope) : -1;
}

int oms_wallet_config_set_origin_header(oms_wallet_sdk_t *sdk, const char *origin_header)
{
    return sdk ? oms_wallet_config_replace(&sdk->config.origin_header, origin_header) : -1;
}

int oms_wallet_config_set_storage_dir(oms_wallet_sdk_t *sdk, const char *storage_dir)
{
    return sdk ? oms_wallet_config_replace(&sdk->config.storage_dir, storage_dir) : -1;
}

int oms_wallet_config_set_auth_signer_provider(
    oms_wallet_sdk_t *sdk,
    const oms_wallet_auth_signer_provider_t *provider)
{
    if (!sdk ||
        !provider ||
        provider->abi_version != OMS_WALLET_AUTH_SIGNER_PROVIDER_ABI_VERSION ||
        !provider->create ||
        !provider->delete_signer ||
        !provider->get_credential ||
        !provider->sign_authorization_message)
    {
        return -1;
    }

    sdk->config.auth_signer_provider = *provider;
    sdk->config.has_auth_signer_provider = 1;
    return 0;
}

int oms_wallet_config_set_session_store_provider(
    oms_wallet_sdk_t *sdk,
    const oms_wallet_session_store_provider_t *provider)
{
    if (!sdk)
    {
        return -1;
    }

    if (!provider)
    {
        memset(&sdk->config.session_store_provider, 0, sizeof(sdk->config.session_store_provider));
        sdk->config.has_session_store_provider = 0;
        return 0;
    }

    if (!provider->write_string ||
        !provider->read_string ||
        !provider->delete_key)
    {
        return -1;
    }

    sdk->config.session_store_provider = *provider;
    sdk->config.has_session_store_provider = 1;
    return 0;
}

int oms_wallet_config_set_max_response_bytes(oms_wallet_sdk_t *sdk, size_t max_response_bytes)
{
    if (!sdk || max_response_bytes == 0)
    {
        return -1;
    }

    sdk->config.max_response_bytes = max_response_bytes;
    return 0;
}

int oms_wallet_config_set_transport(
    oms_wallet_sdk_t *sdk,
    oms_wallet_transport_fn transport,
    void *ctx)
{
    if (!sdk)
    {
        return -1;
    }

    sdk->config.transport = transport;
    sdk->config.transport_ctx = ctx;
    sdk->config.has_transport = transport != NULL;
    return 0;
}
