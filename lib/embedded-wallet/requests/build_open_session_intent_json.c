#include "build_open_session_intent_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cJSON *sequence_build_open_session_intent_json(
    const char *email,
    const char *session_id_hex,
    const char *code
) {
    if (!email || !session_id_hex) {
        return NULL;
    }

    /* verifier = "<email>;<sessionId>" */
    size_t verifier_len = strlen(email) + 1 + strlen(session_id_hex) + 1;
    char *verifier = (char *)malloc(verifier_len);
    if (!verifier) return NULL;
    (void)snprintf(verifier, verifier_len, "%s;%s", email, session_id_hex);

    cJSON *data = cJSON_CreateObject();

    if (!data) {
        free(verifier);
        cJSON_Delete(data);
        return NULL;
    }

    cJSON_AddStringToObject(data, "answer", code);
    cJSON_AddBoolToObject(data, "forceCreateAccount", 0 /* false */);
    cJSON_AddStringToObject(data, "identityType", "Email");
    cJSON_AddStringToObject(data, "sessionId", session_id_hex);
    cJSON_AddStringToObject(data, "verifier", verifier);

    free(verifier);
    return data;
}
