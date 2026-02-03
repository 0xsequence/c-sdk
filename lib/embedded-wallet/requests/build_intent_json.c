#include "build_intent_json.h"
#include <cjson/cJSON.h>

char *sequence_build_intent_json(
    cJSON *data,
    const char *name,
    long long issued_at,
    long long expires_at,
    const char *sig_session_id,
    const char *signature_hex
)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *intent = cJSON_CreateObject();
    cJSON *sigs = cJSON_CreateArray();
    cJSON *sig0 = cJSON_CreateObject();

    if (!root || !intent || !data || !sigs || !sig0) {
        cJSON_Delete(root);
        cJSON_Delete(intent);
        cJSON_Delete(sigs);
        cJSON_Delete(sig0);
        return NULL;
    }

    /* root.intent */
    cJSON_AddItemToObject(root, "intent", intent);

    /* intent */
    cJSON_AddItemToObject(intent, "data", data);
    cJSON_AddStringToObject(intent, "name", name);
    cJSON_AddNumberToObject(intent, "expiresAt", (double)expires_at);
    cJSON_AddNumberToObject(intent, "issuedAt", (double)issued_at);
    cJSON_AddItemToObject(intent, "signatures", sigs);
    cJSON_AddStringToObject(intent, "version", "1 (C 0.1.0)");

    // intent.sigs
    cJSON_AddItemToArray(sigs, sig0);
    cJSON_AddStringToObject(sig0, "sessionId", sig_session_id);
    cJSON_AddStringToObject(sig0, "signature", signature_hex);

    char *json_out = cJSON_PrintUnformatted(root);

    cJSON_Delete(root);
    return json_out; /* caller must free() */
}