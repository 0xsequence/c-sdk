#include "open_session_intent_return.h"

#include <_string.h>
#include <cjson/cJSON.h>

static char *dup_json_string(cJSON *obj) {
    return cJSON_IsString(obj) ? strdup(obj->valuestring) : NULL;
}

SequenceSessionOpenedResult sequence_build_open_session_intent_return(const char *json)
{
    SequenceSessionOpenedResult res = {0};

    cJSON *root = cJSON_Parse(json);
    if (!root) return res;

    /* ---------- session ---------- */
    cJSON *session = cJSON_GetObjectItemCaseSensitive(root, "session");
    if (!cJSON_IsObject(session)) goto cleanup;

    cJSON *identity = cJSON_GetObjectItemCaseSensitive(session, "identity");
    if (!cJSON_IsObject(identity)) goto cleanup;

    res.session.id           = dup_json_string(cJSON_GetObjectItem(session, "id"));
    res.session.projectId    = cJSON_GetObjectItem(session, "projectId")->valueint;
    res.session.userId       = dup_json_string(cJSON_GetObjectItem(session, "userId"));
    res.session.friendlyName = dup_json_string(cJSON_GetObjectItem(session, "friendlyName"));
    res.session.createdAt    = dup_json_string(cJSON_GetObjectItem(session, "createdAt"));
    res.session.refreshedAt  = dup_json_string(cJSON_GetObjectItem(session, "refreshedAt"));
    res.session.expiresAt    = dup_json_string(cJSON_GetObjectItem(session, "expiresAt"));

    res.session.identity.type  = dup_json_string(cJSON_GetObjectItem(identity, "type"));
    res.session.identity.sub   = dup_json_string(cJSON_GetObjectItem(identity, "sub"));
    res.session.identity.email = dup_json_string(cJSON_GetObjectItem(identity, "email"));

    /* ---------- response ---------- */
    cJSON *response = cJSON_GetObjectItemCaseSensitive(root, "response");
    if (!cJSON_IsObject(response)) goto cleanup;

    cJSON *data = cJSON_GetObjectItemCaseSensitive(response, "data");
    if (!cJSON_IsObject(data)) goto cleanup;

    res.responseCode = dup_json_string(cJSON_GetObjectItem(response, "code"));
    res.responseData.sessionId = dup_json_string(cJSON_GetObjectItem(data, "sessionId"));
    res.responseData.wallet    = dup_json_string(cJSON_GetObjectItem(data, "wallet"));

    cleanup:
        cJSON_Delete(root);
    return res;
}
