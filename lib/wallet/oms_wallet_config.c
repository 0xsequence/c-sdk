#include "oms_wallet_config.h"

#include <stdlib.h>
#include <string.h>

#define OMS_WALLET_DEFAULT_API_RPC_URL "https://api.sequence.app/rpc/API"
#define OMS_WALLET_DEFAULT_WALLET_RPC_URL "https://d1sctl7y41hot5.cloudfront.net/rpc/Wallet"
#define OMS_WALLET_DEFAULT_WALLET_AUTH_SCOPE "proj_1"
#define OMS_WALLET_DEFAULT_ORIGIN_HEADER "Origin: http://localhost:3000"

oms_wallet_config_t oms_wallet_config = {0};

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

void oms_wallet_config_cleanup(void)
{
    free(oms_wallet_config.access_key);
    free(oms_wallet_config.indexer_url_template);
    free(oms_wallet_config.api_rpc_url);
    free(oms_wallet_config.wallet_rpc_url);
    free(oms_wallet_config.wallet_auth_scope);
    free(oms_wallet_config.origin_header);
    free(oms_wallet_config.storage_dir);
    memset(&oms_wallet_config, 0, sizeof(oms_wallet_config));
}

int oms_wallet_config_init(const char *access_key)
{
    oms_wallet_config_cleanup();

    if (oms_wallet_config_set_access_key(access_key) != 0 ||
        oms_wallet_config_set_api_rpc_url(OMS_WALLET_DEFAULT_API_RPC_URL) != 0 ||
        oms_wallet_config_set_wallet_rpc_url(OMS_WALLET_DEFAULT_WALLET_RPC_URL) != 0 ||
        oms_wallet_config_set_wallet_auth_scope(OMS_WALLET_DEFAULT_WALLET_AUTH_SCOPE) != 0 ||
        oms_wallet_config_set_origin_header(OMS_WALLET_DEFAULT_ORIGIN_HEADER) != 0)
    {
        oms_wallet_config_cleanup();
        return -1;
    }

    return 0;
}

int oms_wallet_config_set_access_key(const char *access_key)
{
    return oms_wallet_config_replace(&oms_wallet_config.access_key, access_key);
}

int oms_wallet_config_set_indexer_url_template(const char *indexer_url_template)
{
    return oms_wallet_config_replace(&oms_wallet_config.indexer_url_template, indexer_url_template);
}

int oms_wallet_config_set_api_rpc_url(const char *api_rpc_url)
{
    return oms_wallet_config_replace(&oms_wallet_config.api_rpc_url, api_rpc_url);
}

int oms_wallet_config_set_wallet_rpc_url(const char *wallet_rpc_url)
{
    return oms_wallet_config_replace(&oms_wallet_config.wallet_rpc_url, wallet_rpc_url);
}

int oms_wallet_config_set_wallet_auth_scope(const char *wallet_auth_scope)
{
    return oms_wallet_config_replace(&oms_wallet_config.wallet_auth_scope, wallet_auth_scope);
}

int oms_wallet_config_set_origin_header(const char *origin_header)
{
    return oms_wallet_config_replace(&oms_wallet_config.origin_header, origin_header);
}

int oms_wallet_config_set_storage_dir(const char *storage_dir)
{
    return oms_wallet_config_replace(&oms_wallet_config.storage_dir, storage_dir);
}
