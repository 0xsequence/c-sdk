#ifndef OMS_WALLET_INTERNAL_H
#define OMS_WALLET_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "generated/waas/waas.gen.h"
#include "wallet/oms_wallet_auth_signer.h"
#include "wallet/oms_wallet_config.h"

typedef int (*oms_wallet_prepare_request_fn)(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error
);

typedef int (*oms_wallet_parse_response_fn)(
    const waas_http_response *http_response,
    void *response,
    waas_error *error
);

typedef struct {
    oms_wallet_sdk_t *sdk;
    waas_wallet_client *client;
    waas_prepared_request prepared_request;
    waas_http_response http_response;
    waas_error error;
} oms_wallet_rpc_context;

void clear_current_signer(oms_wallet_sdk_t *sdk);
void oms_wallet_set_waas_error(
    waas_error *error,
    const char *name,
    const char *message,
    const char *cause
);
int oms_wallet_parse_wallet_type(
    const char *wallet_type,
    waas_wallet_type *out,
    waas_error *error
);
void oms_wallet_log_waas_error(const char *operation, const waas_error *error);
const oms_wallet_auth_signer_provider_t *oms_wallet_get_auth_signer_provider(oms_wallet_sdk_t *sdk);
int oms_wallet_auth_signer_create(oms_wallet_sdk_t *sdk, char **out_signer_id);
int oms_wallet_auth_signer_delete(oms_wallet_sdk_t *sdk, const char *signer_id);
int oms_wallet_auth_signer_get_credential(
    oms_wallet_sdk_t *sdk,
    const char *signer_id,
    char **out_key_type,
    char **out_credential);
int oms_wallet_auth_signer_sign_authorization_message(
    oms_wallet_sdk_t *sdk,
    const char *signer_id,
    const uint8_t *message,
    size_t message_len,
    char **out_signature_hex);
void oms_wallet_auth_signer_free_string(oms_wallet_sdk_t *sdk, char *value);
int oms_wallet_session_write_string(oms_wallet_sdk_t *sdk, const char *key, const char *value);
int oms_wallet_session_read_string(oms_wallet_sdk_t *sdk, const char *key, char **out_value);
int oms_wallet_session_delete(oms_wallet_sdk_t *sdk, const char *key);
void oms_wallet_session_free_string(oms_wallet_sdk_t *sdk, char *value);
int oms_wallet_session_status_is_not_found(oms_wallet_sdk_t *sdk, int status);
int oms_wallet_authorize_prepared_request(
    oms_wallet_sdk_t *sdk,
    waas_prepared_request *prepared_request,
    waas_error *error);

void oms_wallet_rpc_context_init(oms_wallet_rpc_context *rpc, oms_wallet_sdk_t *sdk);
void oms_wallet_rpc_context_free(oms_wallet_rpc_context *rpc);
int oms_wallet_rpc_execute(
    oms_wallet_rpc_context *rpc,
    const void *request,
    void *response,
    oms_wallet_prepare_request_fn prepare_request,
    oms_wallet_parse_response_fn parse_response
);

int oms_wallet_require_signer_initialized(oms_wallet_sdk_t *sdk);
int oms_wallet_finalize_wallet_response(
    oms_wallet_rpc_context *rpc,
    waas_wallet **wallet_out,
    waas_wallet **response_wallet,
    const char *operation
);
int oms_wallet_prepare_wallet_target_params(
    oms_wallet_sdk_t *sdk,
    const char *chain_id,
    char **network_field,
    char **wallet_id_field,
    waas_error *error,
    const char *operation
);

int oms_wallet_commit_verifier_prepare_request(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error
);
int oms_wallet_commit_verifier_parse_response(
    const waas_http_response *http_response,
    void *response,
    waas_error *error
);
int oms_wallet_complete_auth_prepare_request(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error
);
int oms_wallet_complete_auth_parse_response(
    const waas_http_response *http_response,
    void *response,
    waas_error *error
);
int oms_wallet_create_wallet_prepare_request(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error
);
int oms_wallet_create_wallet_parse_response(
    const waas_http_response *http_response,
    void *response,
    waas_error *error
);
int oms_wallet_use_wallet_prepare_request(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error
);
int oms_wallet_use_wallet_parse_response(
    const waas_http_response *http_response,
    void *response,
    waas_error *error
);
int oms_wallet_sign_message_prepare_request(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error
);
int oms_wallet_sign_message_parse_response(
    const waas_http_response *http_response,
    void *response,
    waas_error *error
);
int oms_wallet_prepare_ethereum_transaction_prepare_request(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error
);
int oms_wallet_prepare_ethereum_transaction_parse_response(
    const waas_http_response *http_response,
    void *response,
    waas_error *error
);
int oms_wallet_execute_prepare_request(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error
);
int oms_wallet_execute_parse_response(
    const waas_http_response *http_response,
    void *response,
    waas_error *error
);
int oms_wallet_get_transaction_status_prepare_request(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error
);
int oms_wallet_get_transaction_status_parse_response(
    const waas_http_response *http_response,
    void *response,
    waas_error *error
);

#endif
