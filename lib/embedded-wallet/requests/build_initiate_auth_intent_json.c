#include "build_initiate_auth_intent_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

cJSON *sequence_build_initiate_auth_intent_json(
    const char *email,
    const char *metadata,
    const char *session_id_hex
) {
    /* verifier = "<email>;<sessionId>" */
    size_t verifier_len = strlen(email) + 1 + strlen(session_id_hex) + 1;
    char *verifier = (char *)malloc(verifier_len);
    if (!verifier) return NULL;
    (void)snprintf(verifier, verifier_len, "%s;%s", email, session_id_hex);

    cJSON *data = cJSON_CreateObject();

    cJSON_AddStringToObject(data, "identityType", "Email");
    cJSON_AddStringToObject(data, "sessionId", session_id_hex);
    cJSON_AddStringToObject(data, "verifier", verifier);
    cJSON_AddStringToObject(data, "metadata", metadata);

    free(verifier);

    return data; /* caller must free() */
}
