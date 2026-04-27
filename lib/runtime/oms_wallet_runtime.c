#include "runtime/oms_wallet_runtime.h"

#include <stddef.h>

static size_t g_oms_wallet_runtime_ref_count = 0;

int oms_wallet_runtime_acquire(waas_error *error)
{
    /* The generated WAAS runtime is the load-bearing curl owner for both
     * handwritten HTTP calls and wallet RPC requests. */
    if (g_oms_wallet_runtime_ref_count == 0 && waas_runtime_init(error) != 0)
    {
        return -1;
    }

    g_oms_wallet_runtime_ref_count++;
    return 0;
}

void oms_wallet_runtime_release(void)
{
    if (g_oms_wallet_runtime_ref_count == 0)
    {
        return;
    }

    g_oms_wallet_runtime_ref_count--;
    if (g_oms_wallet_runtime_ref_count == 0)
    {
        waas_runtime_cleanup();
    }
}
