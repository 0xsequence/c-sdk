#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

static int is_nonempty(const char *s) {
    return (s != NULL && s[0] != '\0');
}

char *sequence_build_initiate_auth_intent_json(
    const char *email,
    const char *metadata,
    const char *session_id_hex,
    long long issued_at,
    long long expires_at,
    const char *sig_session_id,
    const char *signature_hex,
    const char *version_str,
    const char *code,
    const char *friendly_name
) {
    if (!email || !session_id_hex || !sig_session_id || !signature_hex || !version_str) {
        return NULL;
    }

    const int is_open_session = is_nonempty(code);

    if (!metadata) metadata = "";
    if (!friendly_name) friendly_name = "";

    /* verifier = "<email>;<sessionId>" */
    size_t verifier_len = strlen(email) + 1 + strlen(session_id_hex) + 1;
    char *verifier = (char *)malloc(verifier_len);
    if (!verifier) return NULL;
    (void)snprintf(verifier, verifier_len, "%s;%s", email, session_id_hex);

    cJSON *root = cJSON_CreateObject();
    cJSON *intent = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();
    cJSON *sigs = cJSON_CreateArray();
    cJSON *sig0 = cJSON_CreateObject();

    if (!root || !intent || !data || !sigs || !sig0) {
        free(verifier);
        cJSON_Delete(root);
        cJSON_Delete(intent);
        cJSON_Delete(data);
        cJSON_Delete(sigs);
        cJSON_Delete(sig0);
        return NULL;
    }

    /* root.intent */
    cJSON_AddItemToObject(root, "intent", intent);

    /* intent.data */
    cJSON_AddItemToObject(intent, "data", data);
    cJSON_AddStringToObject(data, "identityType", "Email");
    cJSON_AddStringToObject(data, "sessionId", session_id_hex);
    cJSON_AddStringToObject(data, "verifier", verifier);

    if (is_open_session) {
        /* openSession variant */
        cJSON_AddStringToObject(data, "answer", code);
        cJSON_AddBoolToObject(data, "forceCreateAccount", 0 /* false */);

        cJSON_AddStringToObject(intent, "name", "openSession");

        /* top-level friendlyName */
        cJSON_AddStringToObject(root, "friendlyName", friendly_name);
    } else {
        /* initiateAuth variant */
        cJSON_AddStringToObject(data, "metadata", metadata);
        cJSON_AddStringToObject(intent, "name", "initiateAuth");
    }

    /* common intent fields */
    cJSON_AddNumberToObject(intent, "expiresAt", (double)expires_at);
    cJSON_AddNumberToObject(intent, "issuedAt", (double)issued_at);

    cJSON_AddItemToObject(intent, "signatures", sigs);
    cJSON_AddItemToArray(sigs, sig0);
    cJSON_AddStringToObject(sig0, "sessionId", sig_session_id);
    cJSON_AddStringToObject(sig0, "signature", signature_hex);

    cJSON_AddStringToObject(intent, "version", version_str);

    char *json_out = cJSON_PrintUnformatted(root);

    free(verifier);
    cJSON_Delete(root);
    return json_out; /* caller must free() */
}
