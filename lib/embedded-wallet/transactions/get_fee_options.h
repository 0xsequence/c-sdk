#include "embedded-wallet/sequence_wallet.h"
#include "embedded-wallet/transactions/contract_call_data.h"

#include <stdint.h>

typedef struct SequenceFeeToken {
    char *chainId;
    char *contractAddress;
    char *decimals;
    char *logoUrl;
    char *name;
    char *symbol;
    char *tokenId;
    char *type;
} SequenceFeeToken;

typedef struct SequenceFeeOption {
    char *gasLimit;
    char *to;
    char *value;
} SequenceFeeOption;

typedef struct SequenceFeeOptionsResponse {
    char *feeQuote;
    SequenceFeeOption *feeOptions;
    uint32_t len;
} SequenceFeeOptionsResponse;

SequenceFeeOptionsResponse *sequence_get_fee_options(
    sequence_wallet_t *wallet,
    uint64_t chain_id,
    SequenceContractCallData *transactions,
    size_t transactions_len
);
