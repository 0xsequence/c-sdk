#ifndef OMS_WALLET_H
#define OMS_WALLET_H

#include "../generated/waas/waas.gen.h"

typedef waas_wallet oms_wallet_t;
typedef waas_wallet_complete_auth_response oms_wallet_complete_auth_response_t;
typedef waas_wallet_sign_message_response oms_wallet_sign_message_response_t;
typedef waas_wallet_send_transaction_response oms_wallet_send_transaction_response_t;

int oms_wallet_restore_session();

int oms_wallet_start_email_sign_in(const char *email);

oms_wallet_complete_auth_response_t *oms_wallet_complete_email_sign_in(
    const char *code);

const char *oms_wallet_default_wallet_type(void);

oms_wallet_t *oms_wallet_use_wallet(const char *wallet_id);

oms_wallet_t *oms_wallet_create_wallet_of_type(const char *wallet_type);

oms_wallet_t *oms_wallet_create_wallet();

oms_wallet_sign_message_response_t *oms_wallet_sign_message(
    const char *chain_id,
    const char *message);

oms_wallet_send_transaction_response_t *oms_wallet_send_transaction(
    const char *chain_id,
    const char *to,
    const char *value);

void oms_wallet_free(oms_wallet_t *wallet);
void oms_wallet_free_complete_auth(oms_wallet_complete_auth_response_t *response);
void oms_wallet_free_sign_message(oms_wallet_sign_message_response_t *response);
void oms_wallet_free_send_transaction(oms_wallet_send_transaction_response_t *response);

#endif
