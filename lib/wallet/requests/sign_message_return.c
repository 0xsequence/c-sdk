#include "sign_message_return.h"

#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

sequence_sign_message_response *sequence_build_sign_message_return(const char *json) {
    if (!json) {
        return NULL;
    }

    cJSON *root = cJSON_Parse(json);
    if (!root) {
        return NULL;
    }

    sequence_sign_message_response *result = malloc(sizeof(sequence_sign_message_response));
    cJSON *sig = cJSON_GetObjectItemCaseSensitive(root, "signature");
    if (cJSON_IsString(sig) && sig->valuestring) {
        result->signature = strdup(sig->valuestring);
    }

    cJSON_Delete(root);
    return result;
}

void sequence_sign_message_response_free(sequence_sign_message_response *res)
{
    if (!res)
    {
        return;
    }

    free(res->signature);
    res->signature = NULL;
}
