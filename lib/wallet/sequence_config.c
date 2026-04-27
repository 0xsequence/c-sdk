#include "sequence_config.h"

#include <stdlib.h>
#include <string.h>

#define SEQUENCE_DEFAULT_API_RPC_URL "https://api.sequence.app/rpc/API"
#define SEQUENCE_DEFAULT_WALLET_RPC_URL "https://d1sctl7y41hot5.cloudfront.net/rpc/Wallet"
#define SEQUENCE_DEFAULT_WALLET_AUTH_SCOPE "@1:test"
#define SEQUENCE_DEFAULT_ORIGIN_HEADER "Origin: http://localhost:3000"

sequence_config_t sequence_config = {0};

static int sequence_config_replace(char **dst, const char *src)
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

void sequence_config_cleanup(void)
{
    free(sequence_config.access_key);
    free(sequence_config.indexer_url_template);
    free(sequence_config.api_rpc_url);
    free(sequence_config.wallet_rpc_url);
    free(sequence_config.wallet_auth_scope);
    free(sequence_config.origin_header);
    free(sequence_config.storage_dir);
    memset(&sequence_config, 0, sizeof(sequence_config));
}

int sequence_config_init(const char *access_key)
{
    sequence_config_cleanup();

    if (sequence_config_set_access_key(access_key) != 0 ||
        sequence_config_set_api_rpc_url(SEQUENCE_DEFAULT_API_RPC_URL) != 0 ||
        sequence_config_set_wallet_rpc_url(SEQUENCE_DEFAULT_WALLET_RPC_URL) != 0 ||
        sequence_config_set_wallet_auth_scope(SEQUENCE_DEFAULT_WALLET_AUTH_SCOPE) != 0 ||
        sequence_config_set_origin_header(SEQUENCE_DEFAULT_ORIGIN_HEADER) != 0)
    {
        sequence_config_cleanup();
        return -1;
    }

    return 0;
}

int sequence_config_set_access_key(const char *access_key)
{
    return sequence_config_replace(&sequence_config.access_key, access_key);
}

int sequence_config_set_indexer_url_template(const char *indexer_url_template)
{
    return sequence_config_replace(&sequence_config.indexer_url_template, indexer_url_template);
}

int sequence_config_set_api_rpc_url(const char *api_rpc_url)
{
    return sequence_config_replace(&sequence_config.api_rpc_url, api_rpc_url);
}

int sequence_config_set_wallet_rpc_url(const char *wallet_rpc_url)
{
    return sequence_config_replace(&sequence_config.wallet_rpc_url, wallet_rpc_url);
}

int sequence_config_set_wallet_auth_scope(const char *wallet_auth_scope)
{
    return sequence_config_replace(&sequence_config.wallet_auth_scope, wallet_auth_scope);
}

int sequence_config_set_origin_header(const char *origin_header)
{
    return sequence_config_replace(&sequence_config.origin_header, origin_header);
}

int sequence_config_set_storage_dir(const char *storage_dir)
{
    return sequence_config_replace(&sequence_config.storage_dir, storage_dir);
}
