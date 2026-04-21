#ifndef SEQUENCE_RUNTIME_H
#define SEQUENCE_RUNTIME_H

#include "generated/waas/waas.gen.h"

int sequence_runtime_acquire(waas_error *error);
void sequence_runtime_release(void);

#endif
