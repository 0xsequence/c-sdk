#ifndef OMS_WALLET_CONFIG_H
#define OMS_WALLET_CONFIG_H

typedef struct {
    char *access_key;
    char *indexer_url_template;
    char *api_rpc_url;
    char *wallet_rpc_url;
    char *wallet_auth_scope;
    char *origin_header;
    char *storage_dir;
} oms_wallet_config_t;

extern oms_wallet_config_t oms_wallet_config;

int oms_wallet_config_init(const char *access_key);
void oms_wallet_config_cleanup(void);

int oms_wallet_config_set_access_key(const char *access_key);
int oms_wallet_config_set_indexer_url_template(const char *indexer_url_template);
int oms_wallet_config_set_api_rpc_url(const char *api_rpc_url);
int oms_wallet_config_set_wallet_rpc_url(const char *wallet_rpc_url);
int oms_wallet_config_set_wallet_auth_scope(const char *wallet_auth_scope);
int oms_wallet_config_set_origin_header(const char *origin_header);
int oms_wallet_config_set_storage_dir(const char *storage_dir);

#endif
