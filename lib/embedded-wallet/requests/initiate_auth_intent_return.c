#include "initiate_auth_intent_return.h"

#include <_string.h>
#include <cjson/cJSON.h>

SequenceInitiateAuthResponse sequence_build_initiate_auth_intent_return(const char *json) {
    SequenceInitiateAuthResponse resp = (SequenceInitiateAuthResponse){0}; /* ok=0 by default */

    cJSON *root = cJSON_Parse(json);
    if (!root) return resp;

    /* root.response */
    cJSON *wrap = cJSON_GetObjectItemCaseSensitive(root, "response");
    if (!cJSON_IsObject(wrap)) goto cleanup;

    cJSON *code = cJSON_GetObjectItemCaseSensitive(wrap, "code");
    cJSON *data = cJSON_GetObjectItemCaseSensitive(wrap, "data");
    if (!cJSON_IsString(code) || !cJSON_IsObject(data)) goto cleanup;

    cJSON *sessionId    = cJSON_GetObjectItemCaseSensitive(data, "sessionId");
    cJSON *identityType = cJSON_GetObjectItemCaseSensitive(data, "identityType");
    cJSON *expiresIn    = cJSON_GetObjectItemCaseSensitive(data, "expiresIn");
    cJSON *challenge    = cJSON_GetObjectItemCaseSensitive(data, "challenge");

    if (!cJSON_IsString(sessionId) ||
        !cJSON_IsString(identityType) ||
        !cJSON_IsNumber(expiresIn) ||
        !cJSON_IsString(challenge)) {
        goto cleanup;
        }

    resp.code = strdup(code->valuestring);
    resp.data.sessionId = strdup(sessionId->valuestring);
    resp.data.identityType = strdup(identityType->valuestring);
    resp.data.expiresIn = expiresIn->valueint;
    resp.data.challenge = strdup(challenge->valuestring);

    /* If any strdup failed, treat as failure and clean up */
    if (!resp.code || !resp.data.sessionId || !resp.data.identityType || !resp.data.challenge) {
        /* free partial allocations */
        free(resp.code);
        free(resp.data.sessionId);
        free(resp.data.identityType);
        free(resp.data.challenge);
        resp = (SequenceInitiateAuthResponse){0};
        goto cleanup;
    }

    cleanup:
        cJSON_Delete(root);

    return resp;
}
