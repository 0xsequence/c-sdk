#include "uuid.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void generate_uuid_v4(char out[37]) {
    static const char hex[] = "0123456789abcdef";
    int i;

    for (i = 0; i < 36; i++) {
        switch (i) {
        case 8:
        case 13:
        case 18:
        case 23:
            out[i] = '-';
            break;

        case 14:
            // UUID version 4
            out[i] = '4';
            break;

        case 19:
            // UUID variant: 8, 9, a, or b
            out[i] = hex[(rand() & 0x3) | 0x8];
            break;

        default:
            out[i] = hex[rand() & 0xF];
            break;
        }
    }

    out[36] = '\0';
}