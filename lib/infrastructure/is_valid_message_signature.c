#include "is_valid_message_signature.h"

#include <stdio.h>
#include <stdlib.h>

#include <cjson/cJSON.h>

#include "../networking/http_client.h"
#include "../utils/globals.h"
#include "../wallet/sequence_config.h"

static char *build_is_valid_message_signature_json(
    const char *chain_id,
    const char *wallet_address,
    const char *message,
    const char *signature
)
{
    cJSON *root = cJSON_CreateObject();

    if (!root) {
        return NULL;
    }

    cJSON_AddStringToObject(root, "chainId", chain_id);
    cJSON_AddStringToObject(root, "walletAddress", wallet_address);
    cJSON_AddStringToObject(root, "message", message);
    cJSON_AddStringToObject(root, "signature", signature);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

static void parse_is_valid_message_signature_response(
    SequenceIsValidMessageSignatureReturn *out,
    const char *json
)
{
    cJSON *root;
    cJSON *is_valid;

    if (!out || !json) {
        return;
    }

    root = cJSON_Parse(json);
    if (!root) {
        return;
    }

    is_valid = cJSON_GetObjectItemCaseSensitive(root, "isValid");
    if (cJSON_IsBool(is_valid)) {
        out->is_valid = cJSON_IsTrue(is_valid);
    }

    cJSON_Delete(root);
}

SequenceIsValidMessageSignatureReturn *sequence_is_valid_message_signature(
    const char *chain_id,
    const char *wallet_address,
    const char *message,
    const char *signature
)
{
    SequenceIsValidMessageSignatureReturn *out = malloc(sizeof *out);
    HttpClient *c;
    HttpResponse r;
    char *json;

    if (!out) {
        return NULL;
    }

    out->status = -1;
    out->is_valid = false;

    c = http_client_create(sequence_config.api_rpc_url ? sequence_config.api_rpc_url : g_api_rpc_url);
    if (!c) {
        fprintf(stderr, "Failed to create HttpClient\n");
        return out;
    }

    http_add_sequence_access_key(c);
    http_client_add_header(c, "Accept: application/json");

    json = build_is_valid_message_signature_json(chain_id, wallet_address, message, signature);
    if (!json) {
        http_client_destroy(c);
        return out;
    }

    r = http_client_post_json(c, "/IsValidMessageSignature", json, 10000);
    free(json);

    if (r.error) {
        fprintf(stderr, "Request failed: %s\n", r.error);
        http_response_free(&r);
        http_client_destroy(c);
        return out;
    }

    out->status = (int)r.status_code;
    parse_is_valid_message_signature_response(out, r.body);

    http_response_free(&r);
    http_client_destroy(c);
    return out;
}

void free_sequence_is_valid_message_signature_return(SequenceIsValidMessageSignatureReturn *data)
{
    free(data);
}
