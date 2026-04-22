#ifndef OMS_WALLET_RUNTIME_H
#define OMS_WALLET_RUNTIME_H

#include "generated/waas/waas.gen.h"

int oms_wallet_runtime_acquire(waas_error *error);
void oms_wallet_runtime_release(void);

#endif
