#ifndef SEQUENCE_CONNECTOR_H
#define SEQUENCE_CONNECTOR_H

#include "../generated/waas/waas.gen.h"

int sequence_restore_session();

int sequence_sign_in_with_email(const char *email);

waas_wallet_complete_auth_response *sequence_confirm_email_sign_in(
    const char *code);

const char *sequence_default_wallet_type(void);

waas_wallet *sequence_use_wallet(const char *wallet_id);

waas_wallet *sequence_create_wallet_of_type(const char *walletType);

waas_wallet *sequence_create_wallet();

waas_wallet_sign_message_response *sequence_sign_message(
    const char *chain_id,
    const char *message);

waas_wallet_send_transaction_response *sequence_send_transaction(
    const char *chain_id,
    const char *to,
    const char *value);

#endif
