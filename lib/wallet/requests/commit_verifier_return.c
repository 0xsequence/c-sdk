#include "commit_verifier_return.h"

#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

static char* scvr_strdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *out = (char*)malloc(n);
    if (!out) return NULL;
    memcpy(out, s, n);
    return out;
}

static void scvr_set_string(char **dst, const cJSON *obj_item) {
    if (*dst) { free(*dst); *dst = NULL; }
    if (cJSON_IsString(obj_item) && obj_item->valuestring) {
        *dst = scvr_strdup(obj_item->valuestring);
    }
}

sequence_commit_verifier_response *sequence_build_commit_verifier_return(const char *json) {
    if (!json)
    {
        return NULL;
    }

    cJSON *root = cJSON_Parse(json);
    if (!root || !cJSON_IsObject(root)) {
        if (root) cJSON_Delete(root);
        return NULL;
    }

    sequence_commit_verifier_response *resp = calloc(1, sizeof *resp);
    scvr_set_string(&resp->verifier,  cJSON_GetObjectItemCaseSensitive(root, "verifier"));
    scvr_set_string(&resp->loginHint, cJSON_GetObjectItemCaseSensitive(root, "loginHint"));
    scvr_set_string(&resp->challenge, cJSON_GetObjectItemCaseSensitive(root, "challenge"));

    cJSON_Delete(root);
    return resp;
}

void sequence_commit_verifier_response_free(sequence_commit_verifier_response *resp) {
    if (!resp) return;
    free(resp->verifier);
    free(resp->loginHint);
    free(resp->challenge);
    resp->verifier = NULL;
    resp->loginHint = NULL;
    resp->challenge = NULL;
    free(resp);
}
