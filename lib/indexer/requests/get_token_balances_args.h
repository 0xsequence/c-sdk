#ifndef SEQUENCE_GET_TOKEN_BALANCES_ARGS_H
#define SEQUENCE_GET_TOKEN_BALANCES_ARGS_H

#include <stdbool.h>

char *sequence_build_get_token_balances_args(
    const char *contract_address,
    const char *wallet_address,
    bool include_metadata
);

#endif
