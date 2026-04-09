#include "get_token_balances_args.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *sequence_build_get_token_balances_args(
    const char *contract_address,
    const char *account_address,
    bool include_metadata)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *page;
    char *printed;
    char *out;

    if (!root) {
        return NULL;
    }

    if (!cJSON_AddStringToObject(root, "contractAddress", contract_address ? contract_address : "") ||
        !cJSON_AddStringToObject(root, "accountAddress", account_address ? account_address : "") ||
        !cJSON_AddBoolToObject(root, "includeMetadata", include_metadata)) {
        cJSON_Delete(root);
        return NULL;
    }

    page = cJSON_AddObjectToObject(root, "page");
    if (!page ||
        !cJSON_AddNumberToObject(page, "page", 0) ||
        !cJSON_AddNumberToObject(page, "pageSize", 40) ||
        !cJSON_AddBoolToObject(page, "more", 0)) {
        cJSON_Delete(root);
        return NULL;
    }

    printed = cJSON_PrintUnformatted(root);
    out = printed ? strdup(printed) : NULL;
    if (printed) {
        cJSON_free(printed);
    }
    cJSON_Delete(root);
    return out;
}
