#include "is_valid_message_signature.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

#include "../networking/http_client.h"
#include "../wallet/oms_wallet_config.h"

static char *build_is_valid_message_signature_json(
    const char *chain_id,
    const char *wallet_address,
    const char *message,
    const char *signature
)
{
    cJSON *root = cJSON_CreateObject();
    char *printed;
    char *json;

    if (!root) {
        return NULL;
    }

    if (!cJSON_AddStringToObject(root, "chainId", chain_id ? chain_id : "") ||
        !cJSON_AddStringToObject(root, "walletAddress", wallet_address ? wallet_address : "") ||
        !cJSON_AddStringToObject(root, "message", message ? message : "") ||
        !cJSON_AddStringToObject(root, "signature", signature ? signature : "")) {
        cJSON_Delete(root);
        return NULL;
    }

    printed = cJSON_PrintUnformatted(root);
    json = printed ? strdup(printed) : NULL;
    if (printed) {
        cJSON_free(printed);
    }
    cJSON_Delete(root);
    return json;
}

static void parse_is_valid_message_signature_response(
    OmsWalletIsValidMessageSignatureReturn *out,
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

OmsWalletIsValidMessageSignatureReturn *oms_wallet_is_valid_message_signature(
    const char *chain_id,
    const char *wallet_address,
    const char *message,
    const char *signature
)
{
    OmsWalletIsValidMessageSignatureReturn *out = malloc(sizeof *out);
    HttpClient *c;
    HttpResponse r;
    char *json;

    if (!out) {
        return NULL;
    }

    out->status = -1;
    out->is_valid = false;

    if (!oms_wallet_config.api_rpc_url || oms_wallet_config.api_rpc_url[0] == '\0') {
        fprintf(stderr, "oms_wallet_config_init must be called before verify-signature\n");
        return out;
    }

    c = http_client_create(oms_wallet_config.api_rpc_url);
    if (!c) {
        fprintf(stderr, "Failed to create HttpClient\n");
        return out;
    }

    http_add_oms_wallet_access_key(c);
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

void oms_wallet_free_is_valid_message_signature_return(OmsWalletIsValidMessageSignatureReturn *data)
{
    free(data);
}
