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

sequence_wallet_response *sequence_build_wallet_return(const char *json) {
    if (!json) return NULL;

    cJSON *root = cJSON_Parse(json);
    if (!root) return NULL;

    sequence_wallet_response *resp = malloc(sizeof(*resp));
    cJSON *wallet = cJSON_GetObjectItemCaseSensitive(root, "wallet");
    if (cJSON_IsObject(wallet)) {
        resp->wallet.type =
            dup_json_string(cJSON_GetObjectItem(wallet, "type"));

        resp->wallet.address =
            dup_json_string(cJSON_GetObjectItem(wallet, "address"));

        cJSON *index = cJSON_GetObjectItem(wallet, "index");
        if (cJSON_IsNumber(index))
            resp->wallet.index = index->valueint;

        resp->wallet.comment =
            dup_json_string(cJSON_GetObjectItem(wallet, "comment"));
    }

    cJSON_Delete(root);
    return resp;
}

void sequence_wallet_response_free(sequence_wallet_response *data)
{
    if (!data) return;

    free(data->wallet.type);
    free(data->wallet.address);
    free(data->wallet.comment);

    free(data);
}
