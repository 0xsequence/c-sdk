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
    Wallet   wallet;   // first wallet only
} SequenceCompleteAuthResponse;

SequenceCompleteAuthResponse sequence_build_complete_auth_return(const char *json);
