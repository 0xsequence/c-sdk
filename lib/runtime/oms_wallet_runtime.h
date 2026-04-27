#ifndef OMS_WALLET_RUNTIME_H
#define OMS_WALLET_RUNTIME_H

#include "generated/waas/waas.gen.h"

/* Process-global runtime lifecycle for the current single-session SDK design.
 * This is intentionally not thread-safe. */
int oms_wallet_runtime_acquire(waas_error *error);
void oms_wallet_runtime_release(void);

#endif
