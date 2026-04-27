#ifndef OMS_WALLET_INTERNAL_H
#define OMS_WALLET_INTERNAL_H

#include "evm/eoa_wallet.h"
#include "generated/waas/waas.gen.h"

extern eoa_wallet_t *cur_signer;
extern char *cur_challenge;
extern char *cur_verifier;
extern char *cur_wallet_id;

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
    waas_wallet_client *client;
    waas_prepared_request prepared_request;
    waas_http_response http_response;
    waas_error error;
} oms_wallet_rpc_context;

void clear_current_signer(void);
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

void oms_wallet_rpc_context_init(oms_wallet_rpc_context *rpc);
void oms_wallet_rpc_context_free(oms_wallet_rpc_context *rpc);
int oms_wallet_rpc_execute(
    oms_wallet_rpc_context *rpc,
    const void *request,
    void *response,
    oms_wallet_prepare_request_fn prepare_request,
    oms_wallet_parse_response_fn parse_response
);

int oms_wallet_require_signer_initialized(void);
int oms_wallet_finalize_wallet_response(
    oms_wallet_rpc_context *rpc,
    waas_wallet **wallet_out,
    waas_wallet **response_wallet,
    const char *operation
);
int oms_wallet_prepare_wallet_target_params(
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
int oms_wallet_send_transaction_prepare_request(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error
);
int oms_wallet_send_transaction_parse_response(
    const waas_http_response *http_response,
    void *response,
    waas_error *error
);

#endif
