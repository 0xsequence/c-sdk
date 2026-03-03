#include "requests/get_token_balances_return.h"
#include <stdbool.h>
#include <stdint.h>

SequenceGetTokenBalancesReturn *sequence_get_token_balances(
    const char *chain_id,
    const char *contract_address,
    const char *wallet_address,
    bool include_metadata
);

void free_sequence_token_balances_return(
    SequenceGetTokenBalancesReturn *data
);
