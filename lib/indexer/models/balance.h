#ifndef SEQUENCE_BALANCE_H
#define SEQUENCE_BALANCE_H

typedef struct {
    char *contractType;
    char *contractAddress;
    char *accountAddress;
    char *tokenID;
    char *balance;
    char *blockHash;
    int blockNumber;
    int chainId;
} SequenceBalance;

#endif
