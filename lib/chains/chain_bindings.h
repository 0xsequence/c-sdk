#include <stddef.h>

typedef struct {
    const char *key;
    const char *value;
} sequence_chain_data;

const char *sequence_get_chain_name(const char *chain_id);

extern const sequence_chain_data sequence_chain_bindings[];
extern const size_t sequence_chains_count;
