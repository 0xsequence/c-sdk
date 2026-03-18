#include "sequence_wallet.h"

#include <stdlib.h>
#include <string.h>
#include <_string.h>

sequence_wallet *sequence_wallet_from_response(
    const char *address,
    const uint8_t seckey[32])
{
    sequence_wallet *w = calloc(1, sizeof(*w));
    if (!w) return NULL;

    w->address    = address    ? strdup(address)    : NULL;

    if (seckey) {
        memcpy(w->seckey, seckey, 32);
    }

    return w;
}

void sequence_wallet_free(sequence_wallet *w) {
    if (!w) return;

    if (w->address) {
        free(w->address);
    }

    free(w);
}
