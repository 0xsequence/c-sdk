#include "wallet_return.h"

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

SequenceWalletResponse sequence_build_wallet_return(const char *json) {
    SequenceWalletResponse resp = (SequenceWalletResponse){0};
    if (!json) return resp;

    cJSON *root = cJSON_Parse(json);
    if (!root) return resp;

    cJSON *wallet = cJSON_GetObjectItemCaseSensitive(root, "wallet");
    if (cJSON_IsObject(wallet)) {
        resp.wallet.type =
            dup_json_string(cJSON_GetObjectItem(wallet, "type"));

        resp.wallet.address =
            dup_json_string(cJSON_GetObjectItem(wallet, "address"));

        cJSON *index = cJSON_GetObjectItem(wallet, "index");
        if (cJSON_IsNumber(index))
            resp.wallet.index = index->valueint;

        resp.wallet.comment =
            dup_json_string(cJSON_GetObjectItem(wallet, "comment"));
    }

    cJSON_Delete(root);
    return resp;
}
