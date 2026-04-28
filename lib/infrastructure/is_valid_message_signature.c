#include "is_valid_message_signature.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

#ifndef OMS_WALLET_NO_CURL_TRANSPORT
#include "../networking/http_client.h"
#endif

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

static int add_access_headers(oms_wallet_sdk_t *sdk, waas_prepared_request *request)
{
    char *access_header = NULL;
    int rc;

    if (!sdk || !request)
    {
        return -1;
    }

    if (sdk->config.access_key && sdk->config.access_key[0] != '\0')
    {
        size_t len = strlen("X-Access-Key: ") + strlen(sdk->config.access_key) + 1;

        access_header = malloc(len);
        if (!access_header)
        {
            return -1;
        }
        snprintf(access_header, len, "X-Access-Key: %s", sdk->config.access_key);
        rc = waas_prepared_request_add_header(request, access_header);
        free(access_header);
        if (rc != 0)
        {
            return -1;
        }
    }

    return waas_prepared_request_add_header(request, "Accept: application/json");
}

static int verify_with_transport(
    oms_wallet_sdk_t *sdk,
    const char *json,
    OmsWalletIsValidMessageSignatureReturn *out)
{
    waas_prepared_request request;
    waas_http_response response;
    waas_error error;
    int rc = -1;

    waas_prepared_request_init(&request);
    waas_http_response_init(&response);
    waas_error_init(&error);

    request.http_method = waas_strdup("POST");
    request.path = waas_strdup("/IsValidMessageSignature");
    request.body = waas_strdup(json);
    request.body_len = json ? strlen(json) : 0;
    request.content_type = waas_strdup("application/json");

    if (!request.http_method ||
        !request.path ||
        !request.body ||
        !request.content_type ||
        add_access_headers(sdk, &request) != 0)
    {
        goto cleanup;
    }

    if (sdk->config.transport(
            sdk->config.transport_ctx,
            sdk->config.api_rpc_url,
            &request,
            &response,
            &error) != 0)
    {
        if (error.message)
        {
            fprintf(stderr, "IsValidMessageSignature failed: %s\n", error.message);
        }
        goto cleanup;
    }

    out->status = (int)response.status_code;
    parse_is_valid_message_signature_response(out, response.body);
    rc = 0;

cleanup:
    waas_error_free(&error);
    waas_http_response_free(&response);
    waas_prepared_request_free(&request);
    return rc;
}

OmsWalletIsValidMessageSignatureReturn *oms_wallet_is_valid_message_signature(
    oms_wallet_sdk_t *sdk,
    const char *chain_id,
    const char *wallet_address,
    const char *message,
    const char *signature
)
{
    OmsWalletIsValidMessageSignatureReturn *out = malloc(sizeof *out);
    char *json;

    if (!out) {
        return NULL;
    }

    out->status = -1;
    out->is_valid = false;

    if (!sdk || !sdk->config.api_rpc_url || sdk->config.api_rpc_url[0] == '\0') {
        fprintf(stderr, "oms_wallet_sdk_init must be called before verify-signature\n");
        return out;
    }

    json = build_is_valid_message_signature_json(chain_id, wallet_address, message, signature);
    if (!json) {
        return out;
    }

    if (sdk->config.has_transport)
    {
        verify_with_transport(sdk, json, out);
    }
    else
    {
#ifdef OMS_WALLET_NO_CURL_TRANSPORT
        fprintf(stderr, "No SDK transport configured and built-in curl transport is disabled\n");
#else
        HttpClient *c;
        HttpResponse r;

        c = http_client_create(sdk->config.api_rpc_url);
        if (!c) {
            fprintf(stderr, "Failed to create HttpClient\n");
            free(json);
            return out;
        }

        http_add_oms_wallet_access_key(c, sdk->config.access_key);
        http_client_add_header(c, "Accept: application/json");

        r = http_client_post_json(
            c,
            "/IsValidMessageSignature",
            json,
            10000,
            sdk->config.max_response_bytes);

        if (r.error) {
            fprintf(stderr, "Request failed: %s\n", r.error);
            http_response_free(&r);
            http_client_destroy(c);
            free(json);
            return out;
        }

        out->status = (int)r.status_code;
        parse_is_valid_message_signature_response(out, r.body);

        http_response_free(&r);
        http_client_destroy(c);
#endif
    }

    free(json);
    return out;
}

void oms_wallet_free_is_valid_message_signature_return(OmsWalletIsValidMessageSignatureReturn *data)
{
    free(data);
}
