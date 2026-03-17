#include "chain_bindings.h"

#include <string.h>

const sequence_chain_data sequence_chain_bindings[] = {
    { "137", "polygon" },
    { "80002", "amoy" },
};

const size_t sequence_chains_count =
    sizeof(sequence_chain_bindings) / sizeof(sequence_chain_bindings[0]);

const char *sequence_get_chain_name(const char *chain_id)
{
    for (size_t i = 0; i < sequence_chains_count; ++i) {
        if (strcmp(sequence_chain_bindings[i].key, chain_id) == 0) {
            return sequence_chain_bindings[i].value;
        }
    }
    return "undefined";
}
