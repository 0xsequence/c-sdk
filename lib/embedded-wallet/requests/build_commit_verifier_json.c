#include "build_open_session_intent_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *sequence_build_commit_verifier_json(
    const char *email
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
    cJSON_AddStringToObject(params, "handle", email);

    char *json_out = cJSON_PrintUnformatted(root);

    cJSON_Delete(root);
    return json_out;
}
