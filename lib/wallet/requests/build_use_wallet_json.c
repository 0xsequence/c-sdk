#include "build_use_wallet_json.h"

#include <cjson/cJSON.h>

char *sequence_build_use_wallet_json(
    const char *walletType
) {
    cJSON *root = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();

    if (!root || !params) {
        cJSON_Delete(root);
        cJSON_Delete(params);
        return NULL;
    }

    cJSON_AddItemToObject(root, "params", params);

    cJSON_AddStringToObject(params, "walletType", walletType);
    cJSON_AddNumberToObject(params, "walletIndex", 0);

    char *json_out = cJSON_PrintUnformatted(root);

    cJSON_Delete(root);
    return json_out;
}
