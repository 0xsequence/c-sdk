#ifndef OMS_WALLET_CHAIN_BINDINGS_H
#define OMS_WALLET_CHAIN_BINDINGS_H

#include <stddef.h>

typedef struct {
    const char *key;
    const char *value;
} oms_wallet_chain_data;

const char *oms_wallet_get_chain_name(const char *chain_id);

extern const oms_wallet_chain_data oms_wallet_chain_bindings[];
extern const size_t oms_wallet_chains_count;

#endif
