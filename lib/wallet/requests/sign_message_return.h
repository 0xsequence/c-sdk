typedef struct {
    char *signature;
} sequence_sign_message_response;

sequence_sign_message_response *sequence_build_sign_message_return(const char *json);

void sequence_sign_message_response_free(sequence_sign_message_response *res);
