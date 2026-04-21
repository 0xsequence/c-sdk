#ifndef SEQUENCE_CONFIG_H
#define SEQUENCE_CONFIG_H

typedef struct {
    char *access_key;
    char *indexer_url_template;
    char *api_rpc_url;
    char *wallet_rpc_url;
    char *wallet_auth_scope;
    char *origin_header;
    char *storage_dir;
} sequence_config_t;

extern sequence_config_t sequence_config;

int sequence_config_init(const char *access_key);
void sequence_config_cleanup(void);

int sequence_config_set_access_key(const char *access_key);
int sequence_config_set_indexer_url_template(const char *indexer_url_template);
int sequence_config_set_api_rpc_url(const char *api_rpc_url);
int sequence_config_set_wallet_rpc_url(const char *wallet_rpc_url);
int sequence_config_set_wallet_auth_scope(const char *wallet_auth_scope);
int sequence_config_set_origin_header(const char *origin_header);
int sequence_config_set_storage_dir(const char *storage_dir);

#endif
