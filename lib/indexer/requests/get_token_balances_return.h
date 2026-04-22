#ifndef OMS_WALLET_GET_TOKEN_BALANCES_RETURN_H
#define OMS_WALLET_GET_TOKEN_BALANCES_RETURN_H

#include "../models/balance.h"
#include "../models/page.h"

typedef struct {
    int status;
    OmsWalletPage page;
    OmsWalletBalance *balances;
    int balancesCount;
} OmsWalletGetTokenBalancesReturn;

OmsWalletGetTokenBalancesReturn *oms_wallet_build_get_token_balances_return(const char *json);

void log_oms_wallet_get_token_balances_return(const OmsWalletGetTokenBalancesReturn *res);

#endif
