#include "sequence_config.h"

#include <stddef.h>

sequence_config_t sequence_config = {
    .access_key = "",
    .indexer_url_template = NULL,
    .api_rpc_url = NULL
};

void sequence_config_init(const char *new_access_key) {
    sequence_config.access_key = new_access_key;
}

void sequence_config_set_indexer_url_template(const char *indexer_url_template) {
    sequence_config.indexer_url_template = indexer_url_template;
}

void sequence_config_set_api_rpc_url(const char *api_rpc_url) {
    sequence_config.api_rpc_url = api_rpc_url;
}
