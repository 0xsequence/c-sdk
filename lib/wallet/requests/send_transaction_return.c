#include "send_transaction_return.h"

#include <stdlib.h>
#include <_string.h>
#include <cjson/cJSON.h>

sequence_send_transaction_response *sequence_build_send_transaction_return(const char *json) {
    cJSON *root = cJSON_Parse(json);
    if (!root) return NULL;

    sequence_send_transaction_response *resp =
        malloc(sizeof(sequence_send_transaction_response));
    if (!resp) {
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *response = cJSON_GetObjectItemCaseSensitive(root, "response");
    cJSON *txHash = cJSON_GetObjectItemCaseSensitive(response, "txHash");

    if (!cJSON_IsString(txHash)) {
        cJSON_Delete(root);
        free(resp);
        return NULL;
    }

    resp->response.txHash = strdup(txHash->valuestring);

    cJSON_Delete(root);
    return resp;
}

void sequence_send_transaction_response_free(sequence_send_transaction_response *data)
{
    if (!data) return;
    free(data->response.txHash);
    free(data);
}
