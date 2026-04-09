#ifndef SEQUENCE_GET_TOKEN_BALANCES_RETURN_H
#define SEQUENCE_GET_TOKEN_BALANCES_RETURN_H

#include "../models/balance.h"
#include "../models/page.h"

typedef struct {
    int status;
    SequencePage page;
    SequenceBalance *balances;
    int balancesCount;
} SequenceGetTokenBalancesReturn;

SequenceGetTokenBalancesReturn *sequence_build_get_token_balances_return(const char *json);

void log_sequence_get_token_balances_return(const SequenceGetTokenBalancesReturn *res);

#endif
