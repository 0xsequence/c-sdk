#include "build_send_transaction_json.h"

#include <cjson/cJSON.h>

char *build_send_transaction_json(const char *network, const char *to, const char *value)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();

    if (!root || !params) {
        cJSON_Delete(root);
        cJSON_Delete(params);
        return NULL;
    }

    cJSON_AddItemToObject(root, "params", params);

    cJSON_AddStringToObject(params, "mode", "Relayer");
    cJSON_AddStringToObject(params, "network", network);
    cJSON_AddStringToObject(params, "to", to);
    cJSON_AddStringToObject(params, "value", value);

    char *json_out = cJSON_PrintUnformatted(root);

    cJSON_Delete(root);
    return json_out;
}
