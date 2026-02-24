typedef struct {
    char *type;
    char *address;
    int   index;
    char *comment;
} SequenceWalletData;

typedef struct {
    SequenceWalletData wallet;
} SequenceWalletResponse;

SequenceWalletResponse sequence_build_wallet_return(const char *json);
