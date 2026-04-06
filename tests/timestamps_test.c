#include <stdio.h>
#include <stdlib.h>

#include "utils/timestamps.h"

int main(void)
{
    long long prev = timestamp_next_nonce();

    if (prev < 0) {
        fprintf(stderr, "failed to generate initial nonce\n");
        return 1;
    }

    for (int i = 0; i < 1000; ++i) {
        const long long next = timestamp_next_nonce();
        if (next <= prev) {
            fprintf(stderr, "nonce did not increase: prev=%lld next=%lld\n", prev, next);
            return 1;
        }
        prev = next;
    }

    printf("timestamps_test passed\n");
    return 0;
}
