#include <stdint.h>
#include <cjson/cJSON.h>

cJSON *sequence_build_contract_call_intent_json(
    const char* identifier,
    const char* network,
    const char* wallet,

    const char* tx_to,
    const char* tx_type,
    const char* tx_value,

    const char* data_abi,
    const char* arg0,
    const char* arg1
);
