typedef struct {
    const char *access_key;
    const char *indexer_url_template;
    const char *api_rpc_url;
} sequence_config_t;

extern sequence_config_t sequence_config;

void sequence_config_init(const char *new_access_key);

void sequence_config_set_indexer_url_template(const char *indexer_url_template);

void sequence_config_set_api_rpc_url(const char *api_rpc_url);
