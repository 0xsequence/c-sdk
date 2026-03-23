#include "build_sign_message_json.h"

#include <cjson/cJSON.h>

char *build_sign_message_json(const char *wallet, const char *network, const char *message)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();

    if (!root || !params) {
        cJSON_Delete(root);
        cJSON_Delete(params);
        return NULL;
    }

    cJSON_AddItemToObject(root, "params", params);

    cJSON_AddStringToObject(params, "wallet", wallet);
    cJSON_AddStringToObject(params, "network", network);
    cJSON_AddStringToObject(params, "message", message);

    char *json_out = cJSON_PrintUnformatted(root);

    cJSON_Delete(root);
    return json_out;
}
