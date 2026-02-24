#include <stddef.h>

typedef struct {
    char *type;
    char *sub;
    char *email;
} Identity;

typedef struct {
    char *type;
    char *address;
    int   index;
    char *comment;
} Wallet;

typedef struct {
    Identity identity;
    Wallet  *wallets;       // dynamic array
    size_t   wallet_count;
} SequenceCompleteAuthResponse;

SequenceCompleteAuthResponse sequence_build_complete_auth_return(const char *json);
