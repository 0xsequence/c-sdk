#include "build_complete_auth_json.h"

#include <cjson/cJSON.h>

char *sequence_build_complete_auth_json(
    const char *verifier,
    const char *answer
) {
    cJSON *root = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();

    if (!root || !params) {
        cJSON_Delete(root);
        cJSON_Delete(params);
        return NULL;
    }

    cJSON_AddItemToObject(root, "params", params);

    cJSON_AddStringToObject(params, "identityType", "Email");
    cJSON_AddStringToObject(params, "authMode", "OTP");
    cJSON_AddStringToObject(params, "verifier", verifier);
    cJSON_AddStringToObject(params, "answer", answer);

    char *json_out = cJSON_PrintUnformatted(root);

    cJSON_Delete(root);
    return json_out;
}
