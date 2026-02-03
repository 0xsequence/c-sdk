#include "build_contract_call_intent_json.h"

#include <inttypes.h>
#include <stdio.h>

static cJSON* add_string_or_fail(cJSON* obj, const char* key, const char* val) {
    if (!obj || !key || !val) return NULL;
    cJSON* s = cJSON_CreateString(val);
    if (!s) return NULL;
    cJSON_AddItemToObject(obj, key, s);
    return s;
}

cJSON *sequence_build_contract_call_intent_json(
    const char* identifier,
    const char* network,
    const char* wallet,

    const char* tx_to,
    const char* tx_type,
    const char* tx_value,

    const char* data_abi,
    const char* arg0,
    const char* arg1)
{
    cJSON* root = NULL;
    cJSON* transactions = NULL;
    cJSON* tx0 = NULL;
    cJSON* data = NULL;
    cJSON* args = NULL;

    // Basic parameter validation
    if (!identifier || !network || !wallet ||
        !tx_to || !tx_type || !tx_value ||
        !data_abi || !arg0 || !arg1) {
        return NULL;
        }

    root = cJSON_CreateObject();
    if (!root) goto fail;

    if (!add_string_or_fail(root, "identifier", identifier)) goto fail;
    if (!add_string_or_fail(root, "network", network)) goto fail;

    transactions = cJSON_CreateArray();
    if (!transactions) goto fail;
    cJSON_AddItemToObject(root, "transactions", transactions);

	if (!add_string_or_fail(root, "wallet", wallet)) goto fail;

    tx0 = cJSON_CreateObject();
    if (!tx0) goto fail;
    cJSON_AddItemToArray(transactions, tx0);

    // data object
    data = cJSON_CreateObject();
    if (!data) goto fail;
    cJSON_AddItemToObject(tx0, "data", data);

    if (!add_string_or_fail(data, "abi", data_abi)) goto fail;

    args = cJSON_CreateArray();
    if (!args) goto fail;
    cJSON_AddItemToObject(data, "args", args);

    // args array items
    cJSON* a0 = cJSON_CreateString(arg0);
    cJSON* a1 = cJSON_CreateString(arg1);
    if (!a0 || !a1) {
        if (a0) cJSON_Delete(a0);
        if (a1) cJSON_Delete(a1);
        goto fail;
    }
    cJSON_AddItemToArray(args, a0);
    cJSON_AddItemToArray(args, a1);

    // remaining tx fields
    if (!add_string_or_fail(tx0, "to", tx_to)) goto fail;
    if (!add_string_or_fail(tx0, "type", tx_type)) goto fail;
    if (!add_string_or_fail(tx0, "value", tx_value)) goto fail;

    return root;

    fail:
        // Safe even if partially built; cJSON_Delete handles NULL and recursive cleanup
        cJSON_Delete(root);
    return NULL;
}
