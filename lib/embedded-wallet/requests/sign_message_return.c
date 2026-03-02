#include "sign_message_return.h"

#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

SequenceSignMessageResponse sequence_build_sign_message_return(const char *json) {
    SequenceSignMessageResponse result;
    result.signature = NULL;

    if (!json) {
        return result;
    }

    cJSON *root = cJSON_Parse(json);
    if (!root) {
        return result;
    }

    cJSON *sig = cJSON_GetObjectItemCaseSensitive(root, "signature");
    if (cJSON_IsString(sig) && sig->valuestring) {
        result.signature = strdup(sig->valuestring);
    }

    cJSON_Delete(root);
    return result;
}

void sequence_free_sign_message_response(SequenceSignMessageResponse *res)
{
    if (!res)
    {
        return;
    }

    free(res->signature);
    res->signature = NULL;
}
