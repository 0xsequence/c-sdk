#ifndef OMS_WALLET_BALANCE_H
#define OMS_WALLET_BALANCE_H

typedef struct {
    char *contractType;
    char *contractAddress;
    char *accountAddress;
    char *tokenID;
    char *balance;
    char *blockHash;
    int blockNumber;
    int chainId;
} OmsWalletBalance;

#endif
