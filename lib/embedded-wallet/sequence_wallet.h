#ifndef SEQUENCE_WALLET_H
#define SEQUENCE_WALLET_H

#include <stdint.h>

typedef struct sequence_wallet {
    char *address;
    char *email;
    char *session_id;
    uint8_t seckey[32];
} sequence_wallet_t;

sequence_wallet_t *sequence_wallet_from_response(
    const char *address,
    const char *email,
    const char *session_id,
    const uint8_t seckey[32]
);

void sequence_wallet_free(sequence_wallet_t *w);

#endif // SEQUENCE_WALLET_H