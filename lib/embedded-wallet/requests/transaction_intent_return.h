#ifndef TRANSACTION_INTENT_RETURN_H
#define TRANSACTION_INTENT_RETURN_H

#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

typedef struct {
    char *blockNumber;
    char *id;
    int   index;
    int   logs_is_null;
    char *revertReason; // NULL if JSON null/missing
    char *status;
    char *txnHash;
    char *txHash;
} SequenceTransactionReceipt;

typedef struct {
    char *metaTxHash;
    SequenceTransactionReceipt receipt;
} SequenceTransactionData;

typedef struct {
    char *code;
    SequenceTransactionData data;
} SequenceTransactionResponse;

typedef struct {
    SequenceTransactionResponse response;
} SequenceTransactionResult;

SequenceTransactionResult *sequence_build_transaction_intent_return(char *json);
void sequence_free_transaction_result(SequenceTransactionResult *r);

#endif
