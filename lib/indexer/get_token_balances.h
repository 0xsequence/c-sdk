#ifndef OMS_WALLET_GET_TOKEN_BALANCES_H
#define OMS_WALLET_GET_TOKEN_BALANCES_H

#include "requests/get_token_balances_return.h"
#include <stdbool.h>
#include <stdint.h>

OmsWalletGetTokenBalancesReturn *oms_wallet_get_token_balances(
    const char *chain_id,
    const char *contract_address,
    const char *wallet_address,
    bool include_metadata
);

void oms_wallet_free_token_balances_return(
    OmsWalletGetTokenBalancesReturn *data
);

#endif
