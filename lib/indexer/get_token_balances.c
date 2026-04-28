#include "get_token_balances.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "requests/get_token_balances_args.h"
#include "../chains/chain_bindings.h"
#include "../utils/string_utils.h"
#include "../wallet/oms_wallet_internal.h"

#ifndef OMS_WALLET_NO_CURL_TRANSPORT
#include "../networking/http_client.h"
#endif

static OmsWalletGetTokenBalancesReturn *make_error_return(void)
{
    OmsWalletGetTokenBalancesReturn *error_out = malloc(sizeof *error_out);

    if (!error_out)
    {
        return NULL;
    }

    memset(error_out, 0, sizeof(*error_out));
    error_out->status = -1;
    return error_out;
}

static int add_access_headers(oms_wallet_sdk_t *sdk, waas_prepared_request *request)
{
    char *access_header = NULL;
    int rc = 0;

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

static OmsWalletGetTokenBalancesReturn *get_token_balances_with_transport(
    oms_wallet_sdk_t *sdk,
    const char *host,
    char *json)
{
    OmsWalletGetTokenBalancesReturn *parsed = NULL;
    waas_prepared_request request;
    waas_http_response response;
    waas_error error;

    waas_prepared_request_init(&request);
    waas_http_response_init(&response);
    waas_error_init(&error);

    request.http_method = waas_strdup("POST");
    request.path = waas_strdup("/GetTokenBalances");
    request.body = json;
    request.body_len = json ? strlen(json) : 0;
    request.content_type = waas_strdup("application/json");
    json = NULL;

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
            host,
            &request,
            &response,
            &error) != 0)
    {
        if (error.message)
        {
            fprintf(stderr, "GetTokenBalances failed: %s\n", error.message);
        }
        goto cleanup;
    }

    parsed = oms_wallet_build_get_token_balances_return(response.body);

cleanup:
    free(json);
    waas_error_free(&error);
    waas_http_response_free(&response);
    waas_prepared_request_free(&request);
    return parsed;
}

OmsWalletGetTokenBalancesReturn *oms_wallet_get_token_balances(
    oms_wallet_sdk_t *sdk,
    const char *chain_id,
    const char *contract_address,
    const char *wallet_address,
    bool include_metadata
) {
    OmsWalletGetTokenBalancesReturn *error_out;
    OmsWalletGetTokenBalancesReturn *parsed = NULL;
    const char *chain_name = oms_wallet_get_chain_name(chain_id);
    const char *host_template = (sdk && sdk->config.indexer_url_template)
        ? sdk->config.indexer_url_template
        : "https://{value}-indexer.sequence.app/rpc/Indexer/";
    char *host = NULL;
    char *json = NULL;

    error_out = make_error_return();
    if (!error_out)
    {
        return NULL;
    }

    if (!sdk || !chain_name)
    {
        return error_out;
    }

    host = replace_value(host_template, chain_name);
    if (!host) {
        return error_out;
    }

    json = oms_wallet_build_get_token_balances_args(contract_address, wallet_address, include_metadata);
    if (!json) {
        free(host);
        return error_out;
    }

    if (sdk->config.has_transport)
    {
        parsed = get_token_balances_with_transport(sdk, host, json);
        json = NULL;
    }
    else
    {
#ifdef OMS_WALLET_NO_CURL_TRANSPORT
        fprintf(stderr, "No SDK transport configured and built-in curl transport is disabled\n");
#else
        HttpClient *c = http_client_create(host);
        HttpResponse r;

        if (!c) {
            fprintf(stderr, "Failed to create HttpClient\n");
            free(json);
            free(host);
            return error_out;
        }

        http_add_oms_wallet_access_key(c, sdk->config.access_key);
        http_client_add_header(c, "Accept: application/json");

        r = http_client_post_json(c, "/GetTokenBalances", json, 10000, sdk->config.max_response_bytes);

        if (r.error) {
            fprintf(stderr, "Request failed: %s\n", r.error);
            http_response_free(&r);
            http_client_destroy(c);
            free(json);
            free(host);
            return error_out;
        }

        parsed = oms_wallet_build_get_token_balances_return(r.body);

        http_response_free(&r);
        http_client_destroy(c);
#endif
    }

    free(json);
    free(host);
    if (!parsed) {
        return error_out;
    }

    free(error_out);
    return parsed;
}

void oms_wallet_free_token_balances_return(OmsWalletGetTokenBalancesReturn *data) {
    if (!data) return;

    for (int i = 0; i < data->balancesCount; i++) {
        OmsWalletBalance *b = &data->balances[i];
        free(b->contractType);
        free(b->contractAddress);
        free(b->accountAddress);
        free(b->tokenID);
        free(b->balance);
        free(b->blockHash);
    }

    free(data->balances);
    free(data);
}
