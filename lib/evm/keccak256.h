#ifndef OMS_WALLET_KECCAK256_H
#define OMS_WALLET_KECCAK256_H

#include <stdint.h>
#include <stdlib.h>

void keccak256(const uint8_t *in, size_t inlen, uint8_t out32[32]);

#endif
