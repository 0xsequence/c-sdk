typedef struct {
    char *txHash;
} sequence_send_transaction_data;

typedef struct {
    sequence_send_transaction_data response;
} sequence_send_transaction_response;

sequence_send_transaction_response *sequence_build_send_transaction_return(const char *json);

void sequence_send_transaction_response_free(sequence_send_transaction_response *data);
