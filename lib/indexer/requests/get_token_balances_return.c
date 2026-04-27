#include "get_token_balances_return.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *dup_json_string(cJSON *item) {
    const char *value = cJSON_GetStringValue(item);
    return value ? strdup(value) : NULL;
}

void log_oms_wallet_get_token_balances_return(
    const OmsWalletGetTokenBalancesReturn *res
) {
    if (!res) {
        printf("OmsWalletGetTokenBalancesReturn: NULL\n");
        return;
    }

    printf("=== OmsWalletGetTokenBalancesReturn ===\n");

    printf("Page:\n");
    printf("  page      : %d\n", res->page.page);
    printf("  pageSize  : %d\n", res->page.pageSize);
    printf("  more      : %s\n", res->page.more ? "true" : "false");

    printf("Balances (%d):\n", res->balancesCount);

    for (int i = 0; i < res->balancesCount; i++) {
        const OmsWalletBalance *b = &res->balances[i];

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

OmsWalletGetTokenBalancesReturn *oms_wallet_build_get_token_balances_return(const char *json) {
    cJSON *root = cJSON_Parse(json);
    if (!root) {
        return NULL;
    }

    OmsWalletGetTokenBalancesReturn *res = calloc(1, sizeof(OmsWalletGetTokenBalancesReturn));
    cJSON *page = cJSON_GetObjectItemCaseSensitive(root, "page");
    cJSON *balances = cJSON_GetObjectItemCaseSensitive(root, "balances");

    if (!res) {
        cJSON_Delete(root);
        return NULL;
    }

    if (cJSON_IsObject(page)) {
        cJSON *page_number = cJSON_GetObjectItemCaseSensitive(page, "page");
        cJSON *page_size = cJSON_GetObjectItemCaseSensitive(page, "pageSize");
        cJSON *more = cJSON_GetObjectItemCaseSensitive(page, "more");

        if (cJSON_IsNumber(page_number)) {
            res->page.page = (int)cJSON_GetNumberValue(page_number);
        }
        if (cJSON_IsNumber(page_size)) {
            res->page.pageSize = (int)cJSON_GetNumberValue(page_size);
        }
        if (cJSON_IsBool(more)) {
            res->page.more = cJSON_IsTrue(more);
        }
    }

    if (cJSON_IsArray(balances)) {
        res->balancesCount = cJSON_GetArraySize(balances);
        res->balances = calloc((size_t)res->balancesCount, sizeof(OmsWalletBalance));

        if (!res->balances) {
            cJSON_Delete(root);
            free(res);
            return NULL;
        }

        for (int i = 0; i < res->balancesCount; i++) {
            cJSON *b = cJSON_GetArrayItem(balances, i);
            OmsWalletBalance *dst = &res->balances[i];
            cJSON *field = NULL;

            if (!cJSON_IsObject(b)) {
                continue;
            }

            field = cJSON_GetObjectItemCaseSensitive(b, "contractType");
            if (field) dst->contractType = dup_json_string(field);
            field = cJSON_GetObjectItemCaseSensitive(b, "contractAddress");
            if (field) dst->contractAddress = dup_json_string(field);
            field = cJSON_GetObjectItemCaseSensitive(b, "accountAddress");
            if (field) dst->accountAddress = dup_json_string(field);
            field = cJSON_GetObjectItemCaseSensitive(b, "tokenID");
            if (field) dst->tokenID = dup_json_string(field);
            field = cJSON_GetObjectItemCaseSensitive(b, "balance");
            if (field) dst->balance = dup_json_string(field);
            field = cJSON_GetObjectItemCaseSensitive(b, "blockHash");
            if (field) dst->blockHash = dup_json_string(field);
            field = cJSON_GetObjectItemCaseSensitive(b, "blockNumber");
            if (cJSON_IsNumber(field)) dst->blockNumber = (int)cJSON_GetNumberValue(field);
            field = cJSON_GetObjectItemCaseSensitive(b, "chainId");
            if (cJSON_IsNumber(field)) dst->chainId = (int)cJSON_GetNumberValue(field);
        }
    }

    cJSON_Delete(root);
    return res;
}
