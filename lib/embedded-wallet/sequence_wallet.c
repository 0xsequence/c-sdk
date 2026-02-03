#include "sequence_wallet.h"

#include <stdlib.h>
#include <string.h>
#include <_string.h>

sequence_wallet_t *sequence_wallet_from_response(
    const char *address,
    const char *email,
    const char *session_id,
    const uint8_t seckey[32])
{
    sequence_wallet_t *w = calloc(1, sizeof(*w));
    if (!w) return NULL;

    w->address    = address    ? strdup(address)    : NULL;
    w->email      = email      ? strdup(email)      : NULL;
    w->session_id = session_id ? strdup(session_id) : NULL;

    if (seckey) {
        memcpy(w->seckey, seckey, 32);
    }

    return w;
}

void sequence_wallet_free(sequence_wallet_t *w) {
    if (!w) return;

    free(w->address);
    free(w->email);
    free(w->session_id);

    memset(w, 0, sizeof(*w));
}
