#include "get_token_balances.h"

#include <stdio.h>

#include "requests/get_token_balances_args.h"
#include "../chains/chain_bindings.h"
#include "../wallet/sequence_config.h"
#include "../utils/string_utils.h"
#include "../networking/http_client.h"

#include <string.h>
#include <stdlib.h>

SequenceGetTokenBalancesReturn *sequence_get_token_balances(
    const char *chain_id,
    const char *contract_address,
    const char *wallet_address,
    bool include_metadata
) {
    SequenceGetTokenBalancesReturn *error_out;
    SequenceGetTokenBalancesReturn *parsed = NULL;
    const char *chain_name = sequence_get_chain_name(chain_id);
    const char *host_template = sequence_config.indexer_url_template
        ? sequence_config.indexer_url_template
        : "https://{value}-indexer.sequence.app/rpc/Indexer/";
    char *host = NULL;
    HttpClient *c = NULL;
    char *json = NULL;
    HttpResponse r;

    error_out = malloc(sizeof *error_out);
    if (!error_out)
        return NULL;

    error_out->status = -1;
    host = replace_value(host_template, chain_name);
    if (!host) {
        return error_out;
    }

    c = http_client_create(host);
    if (!c) {
        fprintf(stderr, "Failed to create HttpClient\n");
        free(host);
        return error_out;
    }

    http_add_sequence_access_key(c);
    http_client_add_header(c, "Accept: application/json");

    json = sequence_build_get_token_balances_args(contract_address, wallet_address, include_metadata);
    if (!json) {
        http_client_destroy(c);
        free(host);
        return error_out;
    }

    r = http_client_post_json(c, "/GetTokenBalances", json, 10000);
    free(json);
    free(host);

    if (r.error) {
        fprintf(stderr, "Request failed: %s\n", r.error);
        http_response_free(&r);
        http_client_destroy(c);
        return error_out;
    }

    parsed = sequence_build_get_token_balances_return(r.body);

    http_response_free(&r);
    http_client_destroy(c);
    if (!parsed) {
        return error_out;
    }

    free(error_out);
    return parsed;
}

void free_sequence_token_balances_return(SequenceGetTokenBalancesReturn *data) {
    if (!data) return;

    for (int i = 0; i < data->balancesCount; i++) {
        SequenceBalance *b = &data->balances[i];
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
