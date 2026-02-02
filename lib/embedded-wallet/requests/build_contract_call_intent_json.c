#include "build_contract_call_intent_json.h"

#include <inttypes.h>
#include <stdio.h>

cJSON *sequence_build_contract_call_intent_json(
    const uint64_t chain_id,
    const char *wallet,
    const char *abi,
    const char *to,
    const char *value,
    const char *values)
{
    char chain_id_buf[32];
    snprintf(chain_id_buf, sizeof(chain_id_buf), "%" PRIu64, chain_id);

    cJSON *data = cJSON_CreateObject();

    cJSON_AddStringToObject(data, "identifier", "32432");
    cJSON_AddStringToObject(data, "network", chain_id_buf);
    cJSON_AddStringToObject(data, "wallet", wallet);

    return data; /* caller must free() */
}
