#ifndef OMS_WALLET_H
#define OMS_WALLET_H

#include "../generated/waas/waas.gen.h"
#include "wallet/oms_wallet_config.h"

typedef waas_wallet oms_wallet_t;
typedef waas_wallet_complete_auth_response oms_wallet_complete_auth_response_t;
typedef waas_wallet_sign_message_response oms_wallet_sign_message_response_t;
typedef waas_wallet_prepare_ethereum_transaction_response oms_wallet_prepare_ethereum_transaction_response_t;
typedef waas_wallet_execute_response oms_wallet_execute_response_t;
typedef waas_wallet_get_transaction_status_response oms_wallet_get_transaction_status_response_t;

int oms_wallet_restore_session(oms_wallet_sdk_t *sdk);

int oms_wallet_start_email_sign_in(oms_wallet_sdk_t *sdk, const char *email);

oms_wallet_complete_auth_response_t *oms_wallet_complete_email_sign_in(
    oms_wallet_sdk_t *sdk,
    const char *code);

const char *oms_wallet_default_wallet_type(void);

oms_wallet_t *oms_wallet_use_wallet(oms_wallet_sdk_t *sdk, const char *wallet_id);

oms_wallet_t *oms_wallet_create_wallet_of_type(oms_wallet_sdk_t *sdk, const char *wallet_type);

oms_wallet_t *oms_wallet_create_wallet(oms_wallet_sdk_t *sdk);

oms_wallet_sign_message_response_t *oms_wallet_sign_message(
    oms_wallet_sdk_t *sdk,
    const char *chain_id,
    const char *message);

oms_wallet_prepare_ethereum_transaction_response_t *oms_wallet_prepare_ethereum_transaction(
    oms_wallet_sdk_t *sdk,
    const char *chain_id,
    const char *to,
    const char *value);

oms_wallet_execute_response_t *oms_wallet_execute(
    oms_wallet_sdk_t *sdk,
    const char *txn_id);

oms_wallet_get_transaction_status_response_t *oms_wallet_get_transaction_status(
    oms_wallet_sdk_t *sdk,
    const char *txn_id);

void oms_wallet_free(oms_wallet_t *wallet);
void oms_wallet_free_complete_auth(oms_wallet_complete_auth_response_t *response);
void oms_wallet_free_sign_message(oms_wallet_sign_message_response_t *response);
void oms_wallet_free_prepare_ethereum_transaction(oms_wallet_prepare_ethereum_transaction_response_t *response);
void oms_wallet_free_execute(oms_wallet_execute_response_t *response);
void oms_wallet_free_get_transaction_status(oms_wallet_get_transaction_status_response_t *response);

#endif
