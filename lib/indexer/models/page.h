#ifndef OMS_WALLET_PAGE_H
#define OMS_WALLET_PAGE_H

#include <stdbool.h>

typedef struct {
    int page;
    int pageSize;
    bool more;
} OmsWalletPage;

#endif
