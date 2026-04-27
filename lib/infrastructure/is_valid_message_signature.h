#ifndef OMS_WALLET_IS_VALID_MESSAGE_SIGNATURE_H
#define OMS_WALLET_IS_VALID_MESSAGE_SIGNATURE_H

#include <stdbool.h>

typedef struct {
    int status;
    bool is_valid;
} OmsWalletIsValidMessageSignatureReturn;

OmsWalletIsValidMessageSignatureReturn *oms_wallet_is_valid_message_signature(
    const char *chain_id,
    const char *wallet_address,
    const char *message,
    const char *signature
);

void oms_wallet_free_is_valid_message_signature_return(OmsWalletIsValidMessageSignatureReturn *data);

#endif
