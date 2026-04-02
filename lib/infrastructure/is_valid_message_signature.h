#ifndef SEQUENCE_IS_VALID_MESSAGE_SIGNATURE_H
#define SEQUENCE_IS_VALID_MESSAGE_SIGNATURE_H

#include <stdbool.h>

typedef struct {
    int status;
    bool is_valid;
} SequenceIsValidMessageSignatureReturn;

SequenceIsValidMessageSignatureReturn *sequence_is_valid_message_signature(
    const char *chain_id,
    const char *wallet_address,
    const char *message,
    const char *signature
);

void free_sequence_is_valid_message_signature_return(SequenceIsValidMessageSignatureReturn *data);

#endif
