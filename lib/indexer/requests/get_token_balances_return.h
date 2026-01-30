#include "../models/balance.h"
#include "../models/page.h"

typedef struct {
    int status;
    SequencePage page;
    SequenceBalance *balances;
    int balancesCount;
} SequenceGetTokenBalancesReturn;

SequenceGetTokenBalancesReturn *sequence_build_get_token_balances_return(const char *json);
