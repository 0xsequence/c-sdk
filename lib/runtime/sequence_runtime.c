#include "runtime/sequence_runtime.h"

#include <stddef.h>

static size_t g_sequence_runtime_ref_count = 0;

int sequence_runtime_acquire(waas_error *error)
{
    if (g_sequence_runtime_ref_count == 0 && waas_runtime_init(error) != 0)
    {
        return -1;
    }

    g_sequence_runtime_ref_count++;
    return 0;
}

void sequence_runtime_release(void)
{
    if (g_sequence_runtime_ref_count == 0)
    {
        return;
    }

    g_sequence_runtime_ref_count--;
    if (g_sequence_runtime_ref_count == 0)
    {
        waas_runtime_cleanup();
    }
}
