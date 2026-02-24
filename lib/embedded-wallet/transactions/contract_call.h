#include "embedded-wallet/sequence_wallet.h"
#include "embedded-wallet/transactions/contract_call_data.h"

#include <stdint.h>
#include <stddef.h>

char *sequence_contract_call(
	sequence_wallet *wallet,
    uint64_t chain_id,
    SequenceContractCallData *transactions,
    size_t transactions_len
);
