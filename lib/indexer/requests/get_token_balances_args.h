#ifndef OMS_WALLET_GET_TOKEN_BALANCES_ARGS_H
#define OMS_WALLET_GET_TOKEN_BALANCES_ARGS_H

#include <stdbool.h>

char *oms_wallet_build_get_token_balances_args(
    const char *contract_address,
    const char *wallet_address,
    bool include_metadata
);

#endif
