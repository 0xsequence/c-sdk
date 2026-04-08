#include "get_token_balances_return.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *dup_json_string(struct json_object *item) {
    return item && json_object_is_type(item, json_type_string) ? strdup(json_object_get_string(item)) : NULL;
}

void log_sequence_get_token_balances_return(
    const SequenceGetTokenBalancesReturn *res
) {
    if (!res) {
        printf("SequenceGetTokenBalancesReturn: NULL\n");
        return;
    }

    printf("=== SequenceGetTokenBalancesReturn ===\n");

    printf("Page:\n");
    printf("  page      : %d\n", res->page.page);
    printf("  pageSize  : %d\n", res->page.pageSize);
    printf("  more      : %s\n", res->page.more ? "true" : "false");

    printf("Balances (%d):\n", res->balancesCount);

    for (int i = 0; i < res->balancesCount; i++) {
        const SequenceBalance *b = &res->balances[i];

        printf("  [%d]\n", i);
        printf("    contractType    : %s\n", b->contractType ?: "(null)");
        printf("    contractAddress : %s\n", b->contractAddress ?: "(null)");
        printf("    accountAddress  : %s\n", b->accountAddress ?: "(null)");
        printf("    tokenID         : %s\n", b->tokenID ?: "(null)");
        printf("    balance         : %s\n", b->balance ?: "(null)");
        printf("    blockHash       : %s\n", b->blockHash ?: "(null)");
        printf("    blockNumber     : %d\n", b->blockNumber);
        printf("    chainId         : %d\n", b->chainId);
    }

    printf("=====================================\n");
}

SequenceGetTokenBalancesReturn *sequence_build_get_token_balances_return(const char *json) {
    struct json_object *root = json_tokener_parse(json);
    if (!root)
        return NULL;

    SequenceGetTokenBalancesReturn *res = calloc(1, sizeof(SequenceGetTokenBalancesReturn));
    struct json_object *page = NULL;
    struct json_object *balances = NULL;

    if (json_object_object_get_ex(root, "page", &page) && page && json_object_is_type(page, json_type_object)) {
        struct json_object *page_number = NULL;
        struct json_object *page_size = NULL;
        struct json_object *more = NULL;

        if (json_object_object_get_ex(page, "page", &page_number)) {
            res->page.page = json_object_get_int(page_number);
        }
        if (json_object_object_get_ex(page, "pageSize", &page_size)) {
            res->page.pageSize = json_object_get_int(page_size);
        }
        if (json_object_object_get_ex(page, "more", &more)) {
            res->page.more = json_object_get_boolean(more);
        }
    }

    if (json_object_object_get_ex(root, "balances", &balances) && balances &&
        json_object_is_type(balances, json_type_array)) {
        res->balancesCount = (int)json_object_array_length(balances);
        res->balances = calloc(res->balancesCount, sizeof(SequenceBalance));

        for (int i = 0; i < res->balancesCount; i++) {
            struct json_object *b = json_object_array_get_idx(balances, i);
            SequenceBalance *dst = &res->balances[i];
            struct json_object *field = NULL;

            if (!b || !json_object_is_type(b, json_type_object)) {
                continue;
            }

            if (json_object_object_get_ex(b, "contractType", &field)) dst->contractType = dup_json_string(field);
            if (json_object_object_get_ex(b, "contractAddress", &field)) dst->contractAddress = dup_json_string(field);
            if (json_object_object_get_ex(b, "accountAddress", &field)) dst->accountAddress = dup_json_string(field);
            if (json_object_object_get_ex(b, "tokenID", &field)) dst->tokenID = dup_json_string(field);
            if (json_object_object_get_ex(b, "balance", &field)) dst->balance = dup_json_string(field);
            if (json_object_object_get_ex(b, "blockHash", &field)) dst->blockHash = dup_json_string(field);
            if (json_object_object_get_ex(b, "blockNumber", &field)) dst->blockNumber = json_object_get_int(field);
            if (json_object_object_get_ex(b, "chainId", &field)) dst->chainId = json_object_get_int(field);
        }
    }

    json_object_put(root);

    return res;
}
