#include <stdint.h>
#include <cjson/cJSON.h>

cJSON *sequence_build_contract_call_intent_json(
    const uint64_t chain_id,
    const char *wallet,
    const char *abi,
    const char *to,
    const char *value,
    const char *values
);
