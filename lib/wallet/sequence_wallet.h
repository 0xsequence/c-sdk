#ifndef SEQUENCE_WALLET_H
#define SEQUENCE_WALLET_H

#include <stdint.h>

typedef struct {
    char *address;
    uint8_t seckey[32];
} sequence_wallet;

sequence_wallet *sequence_wallet_from_response(
    const char *address,
    const uint8_t seckey[32]
);

void sequence_wallet_free(sequence_wallet *w);

#endif // SEQUENCE_WALLET_H