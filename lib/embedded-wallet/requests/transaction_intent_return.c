#include "transaction_intent_return.h"

static char *dup_json_string(const cJSON *item) {
    if (!cJSON_IsString(item) || item->valuestring == NULL) return NULL;
    size_t n = strlen(item->valuestring);
    char *out = (char *)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, item->valuestring, n + 1);
    return out;
}

void sequence_free_transaction_result(SequenceTransactionResult *r) {
    if (!r) return;

    free(r->response.code);
    free(r->response.data.metaTxHash);

    free(r->response.data.receipt.blockNumber);
    free(r->response.data.receipt.id);
    free(r->response.data.receipt.revertReason);
    free(r->response.data.receipt.status);
    free(r->response.data.receipt.txnHash);
    free(r->response.data.receipt.txHash);

    memset(r, 0, sizeof(*r));
}

SequenceTransactionResult *
sequence_build_transaction_intent_return(char *json) {
    if (!json) return NULL;

    SequenceTransactionResult *r =
        (SequenceTransactionResult *)calloc(1, sizeof(*r));
    if (!r) return NULL;

    cJSON *root = cJSON_Parse(json);
    if (!root) goto fail;

    const cJSON *j_response = cJSON_GetObjectItemCaseSensitive(root, "response");
    if (!cJSON_IsObject(j_response)) goto fail;

    const cJSON *j_code = cJSON_GetObjectItemCaseSensitive(j_response, "code");
    r->response.code = dup_json_string(j_code);
    if (!r->response.code) goto fail;

    const cJSON *j_data = cJSON_GetObjectItemCaseSensitive(j_response, "data");
    if (!cJSON_IsObject(j_data)) goto fail;

    const cJSON *j_meta = cJSON_GetObjectItemCaseSensitive(j_data, "metaTxHash");
    r->response.data.metaTxHash = dup_json_string(j_meta);
    if (!r->response.data.metaTxHash) goto fail;

    const cJSON *j_receipt = cJSON_GetObjectItemCaseSensitive(j_data, "receipt");
    if (!cJSON_IsObject(j_receipt)) goto fail;

    SequenceTransactionReceipt *rc = &r->response.data.receipt;

    rc->blockNumber = dup_json_string(cJSON_GetObjectItemCaseSensitive(j_receipt, "blockNumber"));
    if (!rc->blockNumber) goto fail;

    rc->id = dup_json_string(cJSON_GetObjectItemCaseSensitive(j_receipt, "id"));
    if (!rc->id) goto fail;

    {
        const cJSON *j_index = cJSON_GetObjectItemCaseSensitive(j_receipt, "index");
        if (!cJSON_IsNumber(j_index)) goto fail;
        rc->index = j_index->valueint;
    }

    {
        const cJSON *j_logs = cJSON_GetObjectItemCaseSensitive(j_receipt, "logs");
        rc->logs_is_null = cJSON_IsNull(j_logs) ? 1 : 0;
    }

    {
        const cJSON *j_rr = cJSON_GetObjectItemCaseSensitive(j_receipt, "revertReason");
        rc->revertReason = cJSON_IsString(j_rr) ? dup_json_string(j_rr) : NULL;
    }

    rc->status = dup_json_string(cJSON_GetObjectItemCaseSensitive(j_receipt, "status"));
    if (!rc->status) goto fail;

    rc->txnHash = dup_json_string(cJSON_GetObjectItemCaseSensitive(j_receipt, "txnHash"));
    if (!rc->txnHash) goto fail;

    rc->txHash = dup_json_string(cJSON_GetObjectItemCaseSensitive(j_receipt, "txHash"));
    if (!rc->txHash) goto fail;

    cJSON_Delete(root);
    return r;

fail:
    if (root) cJSON_Delete(root);
    sequence_free_transaction_result(r);
    free(r);
    return NULL;
}
