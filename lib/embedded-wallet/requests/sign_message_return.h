typedef struct {
    char *signature;
} SequenceSignMessageResponse;

SequenceSignMessageResponse sequence_build_sign_message_return(const char *json);

void sequence_free_sign_message_response(SequenceSignMessageResponse *res);
