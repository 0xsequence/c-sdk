typedef struct {
    char *type;
    char *address;
    int   index;
    char *comment;
} sequence_wallet_data;

typedef struct {
    sequence_wallet_data wallet;
} sequence_wallet_response;

sequence_wallet_response *sequence_build_wallet_return(const char *json);

void sequence_wallet_response_free(sequence_wallet_response *data);
