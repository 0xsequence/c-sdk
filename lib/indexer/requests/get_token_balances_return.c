#include "get_token_balances_return.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

static char *dup_json_string(cJSON *item) {
    return item && cJSON_IsString(item) ? strdup(item->valuestring) : NULL;
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
    cJSON *root = cJSON_Parse(json);
    if (!root)
        return NULL;

    SequenceGetTokenBalancesReturn *res = calloc(1, sizeof(SequenceGetTokenBalancesReturn));

    cJSON *page = cJSON_GetObjectItem(root, "page");
    if (page) {
        res->page.page     = cJSON_GetObjectItem(page, "page")->valueint;
        res->page.pageSize = cJSON_GetObjectItem(page, "pageSize")->valueint;
        res->page.more     = cJSON_IsTrue(cJSON_GetObjectItem(page, "more"));
    }

    cJSON *balances = cJSON_GetObjectItem(root, "balances");
    if (balances && cJSON_IsArray(balances)) {
        res->balancesCount = cJSON_GetArraySize(balances);
        res->balances = calloc(res->balancesCount, sizeof(SequenceBalance));

        for (int i = 0; i < res->balancesCount; i++) {
            cJSON *b = cJSON_GetArrayItem(balances, i);
            SequenceBalance *dst = &res->balances[i];

            dst->contractType    = dup_json_string(cJSON_GetObjectItem(b, "contractType"));
            dst->contractAddress = dup_json_string(cJSON_GetObjectItem(b, "contractAddress"));
            dst->accountAddress  = dup_json_string(cJSON_GetObjectItem(b, "accountAddress"));
            dst->tokenID         = dup_json_string(cJSON_GetObjectItem(b, "tokenID"));
            dst->balance         = dup_json_string(cJSON_GetObjectItem(b, "balance"));
            dst->blockHash       = dup_json_string(cJSON_GetObjectItem(b, "blockHash"));
            dst->blockNumber     = cJSON_GetObjectItem(b, "blockNumber")->valueint;
            dst->chainId         = cJSON_GetObjectItem(b, "chainId")->valueint;
        }
    }

    cJSON_Delete(root);
    log_sequence_get_token_balances_return(res);

    return res;
}