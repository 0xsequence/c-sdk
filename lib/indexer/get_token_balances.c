#include "get_token_balances.h"

#include <stdio.h>

#include "requests/get_token_balances_args.h"
#include "../chains/chain_bindings.h"
#include "../utils/string_utils.h"
#include "../networking/http_client.h"

#include <string.h>
#include <stdlib.h>

SequenceGetTokenBalancesReturn *sequence_get_token_balances(
    uint64_t chain_id,
    const char *contract_address,
    const char *wallet_address,
    bool include_metadata
) {
    SequenceGetTokenBalancesReturn *out = malloc(sizeof *out);

    if (!out)
        return NULL;

    out->status = -1;

    const char *chain_name = sequence_get_chain_name(chain_id);
    const char *host = replace_value("https://{value}-indexer.sequence.app/rpc/Indexer/", chain_name);

    HttpClient *c = http_client_create(host);
    if (!c) {
        fprintf(stderr, "Failed to create HttpClient\n");
        return out;
    }

    http_add_sequence_access_key(c);
    http_client_add_header(c, "Accept: application/json");

    char *json = sequence_build_get_token_balances_args(contract_address, wallet_address, include_metadata);

    HttpResponse r = http_client_post_json(c, "/GetTokenBalances", json, 10000);

    if (r.error) {
        fprintf(stderr, "Request failed: %s\n", r.error);
        http_response_free(&r);
        http_client_destroy(c);
        return out;
    }

    printf("Status: %ld\n", r.status_code);
    printf("Body (%zu bytes):\n%s\n", r.body_len, r.body);

    out = sequence_build_get_token_balances_return(r.body);

    http_response_free(&r);
    http_client_destroy(c);

    return out;
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
