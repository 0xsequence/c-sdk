#include "contract_call.h"
#include "embedded-wallet/requests/build_contract_call_intent_json.h"

#include <string.h>
#include <stdlib.h>

SequenceContractCallResult sequence_contract_call(
    uint64_t chain_id,
    const char *contract_address,
    uint64_t value_wei,
    const char *function_signature,
    const Arg *args,
    size_t arg_count
) {
    (void)chain_id;
    (void)contract_address;
    (void)value_wei;
    (void)function_signature;
    (void)args;
    (void)arg_count;

    SequenceContractCallResult result;

    // Explicit empty / not-implemented behavior
    result.payload = NULL;
    result.payload_len = 0;
    result.status = -1;

    // Safe error message
    memset(result.error, 0, sizeof(result.error));
    strncpy(
        result.error,
        "sequence_contract_call not implemented",
        sizeof(result.error) - 1
    );

    return result;
}
