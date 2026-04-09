#ifndef SEQUENCE_WALLET_INTERNAL_H
#define SEQUENCE_WALLET_INTERNAL_H

#include "evm/eoa_wallet.h"
#include "generated/waas/waas.gen.h"

extern eoa_wallet_t *cur_signer;
extern char *cur_challenge;
extern char *cur_verifier;

typedef int (*sequence_prepare_request_fn)(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error
);

typedef int (*sequence_parse_response_fn)(
    const waas_http_response *http_response,
    void *response,
    waas_error *error
);

typedef struct {
    waas_wallet_client *client;
    waas_prepared_request prepared_request;
    waas_http_response http_response;
    waas_error error;
} sequence_wallet_rpc_context;

void clear_current_signer(void);
void sequence_set_waas_error(
    waas_error *error,
    const char *name,
    const char *message,
    const char *cause
);
int sequence_parse_wallet_type(
    const char *wallet_type,
    waas_wallet_type *out,
    waas_error *error
);
void sequence_log_waas_error(const char *operation, const waas_error *error);

void sequence_wallet_rpc_context_init(sequence_wallet_rpc_context *rpc);
void sequence_wallet_rpc_context_free(sequence_wallet_rpc_context *rpc);
int sequence_wallet_rpc_execute(
    sequence_wallet_rpc_context *rpc,
    const void *request,
    void *response,
    sequence_prepare_request_fn prepare_request,
    sequence_parse_response_fn parse_response
);

int sequence_require_signer_initialized(void);
int sequence_finalize_wallet_response(
    sequence_wallet_rpc_context *rpc,
    waas_wallet **wallet_out,
    waas_wallet **response_wallet,
    const char *operation
);
int sequence_prepare_wallet_target_params(
    const char *chain_id,
    char **address_out,
    const char **network_out,
    char **network_field,
    char **wallet_field,
    waas_error *error,
    const char *operation
);

int sequence_commit_verifier_prepare_request(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error
);
int sequence_commit_verifier_parse_response(
    const waas_http_response *http_response,
    void *response,
    waas_error *error
);
int sequence_complete_auth_prepare_request(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error
);
int sequence_complete_auth_parse_response(
    const waas_http_response *http_response,
    void *response,
    waas_error *error
);
int sequence_create_wallet_prepare_request(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error
);
int sequence_create_wallet_parse_response(
    const waas_http_response *http_response,
    void *response,
    waas_error *error
);
int sequence_use_wallet_prepare_request(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error
);
int sequence_use_wallet_parse_response(
    const waas_http_response *http_response,
    void *response,
    waas_error *error
);
int sequence_sign_message_prepare_request(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error
);
int sequence_sign_message_parse_response(
    const waas_http_response *http_response,
    void *response,
    waas_error *error
);
int sequence_send_transaction_prepare_request(
    const void *request,
    waas_prepared_request *prepared_request,
    waas_error *error
);
int sequence_send_transaction_parse_response(
    const waas_http_response *http_response,
    void *response,
    waas_error *error
);

#endif
