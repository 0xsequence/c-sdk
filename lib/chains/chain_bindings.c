#include "chain_bindings.h"

#include <string.h>

const oms_wallet_chain_data oms_wallet_chain_bindings[] = {
    { "137", "polygon" },
    { "80002", "amoy" },
};

const size_t oms_wallet_chains_count =
    sizeof(oms_wallet_chain_bindings) / sizeof(oms_wallet_chain_bindings[0]);

const char *oms_wallet_get_chain_name(const char *chain_id)
{
    for (size_t i = 0; i < oms_wallet_chains_count; ++i) {
        if (strcmp(oms_wallet_chain_bindings[i].key, chain_id) == 0) {
            return oms_wallet_chain_bindings[i].value;
        }
    }
    return "undefined";
}
