#include "complete_auth_return.h"

#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

static char *dup_json_string(const cJSON *item) {
    if (!cJSON_IsString(item) || !item->valuestring) return NULL;
    size_t len = strlen(item->valuestring) + 1;
    char *out = malloc(len);
    if (out) memcpy(out, item->valuestring, len);
    return out;
}

SequenceCompleteAuthResponse sequence_build_complete_auth_return(const char *json) {
    SequenceCompleteAuthResponse resp = {0};
    if (!json) return resp;

    cJSON *root = cJSON_Parse(json);
    if (!root) return resp;

    /* ---- identity ---- */
    cJSON *identity = cJSON_GetObjectItemCaseSensitive(root, "identity");
    if (cJSON_IsObject(identity)) {
        resp.identity.type  = dup_json_string(cJSON_GetObjectItem(identity, "type"));
        resp.identity.sub   = dup_json_string(cJSON_GetObjectItem(identity, "sub"));
        resp.identity.email = dup_json_string(cJSON_GetObjectItem(identity, "email"));
    }

    /* ---- wallets[0] ---- */
    cJSON *wallets = cJSON_GetObjectItemCaseSensitive(root, "wallets");
    if (cJSON_IsArray(wallets)) {
        cJSON *wallet = cJSON_GetArrayItem(wallets, 0);
        if (cJSON_IsObject(wallet)) {
            resp.wallet.type    = dup_json_string(cJSON_GetObjectItem(wallet, "type"));
            resp.wallet.address = dup_json_string(cJSON_GetObjectItem(wallet, "address"));

            cJSON *index = cJSON_GetObjectItem(wallet, "index");
            if (cJSON_IsNumber(index))
                resp.wallet.index = index->valueint;

            resp.wallet.comment = dup_json_string(cJSON_GetObjectItem(wallet, "comment"));
        }
    }

    cJSON_Delete(root);
    return resp;
}
