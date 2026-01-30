#include "get_token_balances_args.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

char *sequence_build_get_token_balances_args(
    const char *contract_address,
    const char *account_address,
    bool include_metadata
)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *page = cJSON_CreateObject();

    if (!root || !page) {
        cJSON_Delete(root);
        cJSON_Delete(page);
        return NULL;
    }

    cJSON_AddStringToObject(root, "contractAddress", contract_address);
    cJSON_AddStringToObject(root, "accountAddress", account_address);
    cJSON_AddBoolToObject(root, "includeMetadata", include_metadata ? 1 : 0);

    cJSON_AddItemToObject(root, "page", page);
    cJSON_AddNumberToObject(page, "page", (double)0);
    cJSON_AddNumberToObject(page, "pageSize", (double)40);
    cJSON_AddBoolToObject(page, "more", false);

    char *json_out = cJSON_PrintUnformatted(root);

    cJSON_Delete(root);
    return json_out;
}