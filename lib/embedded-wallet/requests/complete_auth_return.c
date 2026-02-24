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

    /* ---- wallets[] ---- */
    cJSON *wallets = cJSON_GetObjectItemCaseSensitive(root, "wallets");
    if (cJSON_IsArray(wallets)) {
        resp.wallet_count = (size_t)cJSON_GetArraySize(wallets);
        if (resp.wallet_count > 0) {
            resp.wallets = calloc(resp.wallet_count, sizeof(Wallet));

            for (size_t i = 0; i < resp.wallet_count; ++i) {
                cJSON *wallet = cJSON_GetArrayItem(wallets, (int)i);
                if (!cJSON_IsObject(wallet)) continue;

                resp.wallets[i].type =
                    dup_json_string(cJSON_GetObjectItem(wallet, "type"));

                resp.wallets[i].address =
                    dup_json_string(cJSON_GetObjectItem(wallet, "address"));

                cJSON *index = cJSON_GetObjectItem(wallet, "index");
                if (cJSON_IsNumber(index))
                    resp.wallets[i].index = index->valueint;

                resp.wallets[i].comment =
                    dup_json_string(cJSON_GetObjectItem(wallet, "comment"));
            }
        }
    }

    cJSON_Delete(root);
    return resp;
}
