#include "is_valid_message_signature.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <json-c/json.h>

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
    struct json_object *root = json_object_new_object();

    if (!root) {
        return NULL;
    }

    json_object_object_add(root, "chainId", json_object_new_string(chain_id ? chain_id : ""));
    json_object_object_add(root, "walletAddress", json_object_new_string(wallet_address ? wallet_address : ""));
    json_object_object_add(root, "message", json_object_new_string(message ? message : ""));
    json_object_object_add(root, "signature", json_object_new_string(signature ? signature : ""));

    const char *json_str = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
    char *json = json_str ? strdup(json_str) : NULL;
    json_object_put(root);
    return json;
}

static void parse_is_valid_message_signature_response(
    SequenceIsValidMessageSignatureReturn *out,
    const char *json
)
{
    struct json_object *root;
    struct json_object *is_valid = NULL;

    if (!out || !json) {
        return;
    }

    root = json_tokener_parse(json);
    if (!root) {
        return;
    }

    if (json_object_object_get_ex(root, "isValid", &is_valid) &&
        is_valid && json_object_is_type(is_valid, json_type_boolean)) {
        out->is_valid = json_object_get_boolean(is_valid);
    }

    json_object_put(root);
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
