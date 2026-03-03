#include "send_transaction_return.h"

#include <cjson/cJSON.h>

SequenceSendTransactionResponse sequence_build_send_transaction_return(const char *json) {
    SequenceSendTransactionResponse resp = (SequenceSendTransactionResponse){0};
    if (!json) return resp;

    cJSON *root = cJSON_Parse(json);
    if (!root) return resp;

    cJSON_Delete(root);
    return resp;
}
