#include "build_signable_intent_json.h"
#include "utils/globals.h"

char *build_signable_intent_json(
    cJSON *data,
    const char *name,
    long long issued_at,
    long long expires_at)
{
    cJSON *root = cJSON_CreateObject();

    if (!root || !data) {
        cJSON_Delete(root);
        return NULL;
    }

    /* intent */
    cJSON_AddItemToObject(root, "data", data);
    cJSON_AddNumberToObject(root, "expiresAt", (double)expires_at);
    cJSON_AddNumberToObject(root, "issuedAt", (double)issued_at);
    cJSON_AddStringToObject(root, "name", name);
    cJSON_AddStringToObject(root, "version", g_sequence_sdk_version);

    char *json_out = cJSON_PrintUnformatted(root);

    cJSON_Delete(root);
    return json_out; /* caller must free() */
}
