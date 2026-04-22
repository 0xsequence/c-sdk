#include "oms_wallet.h"
#include "oms_wallet_internal.h"

#include <stdio.h>
#include <stdlib.h>

#include "chains/chain_bindings.h"
#include "storage/secure_storage.h"

static void *oms_wallet_alloc_rpc_response(
    size_t size,
    void (*init_response)(void *),
    oms_wallet_rpc_context *rpc,
    const char *message
)
{
    void *response = calloc(1, size);

    if (!response)
    {
        oms_wallet_set_waas_error(
            &rpc->error,
            "ClientError",
            message,
            NULL);
        return NULL;
    }

    init_response(response);
    return response;
}

static void *oms_wallet_finish_rpc_response(
    void *response,
    void (*free_response)(void *),
    oms_wallet_rpc_context *rpc,
    const char *operation
)
{
    if (!response || rpc->error.message)
    {
        oms_wallet_log_waas_error(operation, &rpc->error);
    }

    if (rpc->error.message)
    {
        if (response)
        {
            free_response(response);
            free(response);
        }
        return NULL;
    }

    return response;
}

oms_wallet_t *oms_wallet_use_wallet(const char *wallet_id)
{
    waas_use_wallet_request params;
    waas_wallet_use_wallet_request request;
    waas_wallet_use_wallet_response response;
    oms_wallet_rpc_context rpc;
    waas_wallet *wallet = NULL;

    if (!oms_wallet_require_signer_initialized())
    {
        return NULL;
    }

    waas_use_wallet_request_init(&params);
    waas_wallet_use_wallet_request_init(&request);
    waas_wallet_use_wallet_response_init(&response);
    oms_wallet_rpc_context_init(&rpc);

    if (!wallet_id || wallet_id[0] == '\0')
    {
        oms_wallet_set_waas_error(
            &rpc.error,
            "ClientError",
            "wallet id is required",
            NULL);
        goto cleanup;
    }

    params.wallet_id = waas_strdup(wallet_id);
    if (wallet_id && !params.wallet_id)
    {
        oms_wallet_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to prepare UseWallet request",
            NULL);
        goto cleanup;
    }
    request.use_wallet_request = &params;

    if (oms_wallet_rpc_execute(
            &rpc,
            &request,
            &response,
            oms_wallet_use_wallet_prepare_request,
            oms_wallet_use_wallet_parse_response) != 0)
    {
        goto cleanup;
    }

    if (oms_wallet_finalize_wallet_response(
            &rpc,
            &wallet,
            response.use_wallet_response ? &response.use_wallet_response->wallet : NULL,
            "failed to extract UseWallet response") != 0)
    {
        goto cleanup;
    }

cleanup:
    if (!wallet)
    {
        oms_wallet_log_waas_error("UseWallet", &rpc.error);
    }

    oms_wallet_rpc_context_free(&rpc);
    waas_wallet_use_wallet_response_free(&response);
    waas_use_wallet_request_free(&params);
    return wallet;
}

oms_wallet_t *oms_wallet_create_wallet()
{
    return oms_wallet_create_wallet_of_type(oms_wallet_default_wallet_type());
}

oms_wallet_t *oms_wallet_create_wallet_of_type(const char *wallet_type)
{
    waas_create_wallet_request params;
    waas_wallet_create_wallet_request request;
    waas_wallet_create_wallet_response response;
    oms_wallet_rpc_context rpc;
    waas_wallet *wallet = NULL;

    if (!wallet_type)
    {
        wallet_type = oms_wallet_default_wallet_type();
    }

    if (!oms_wallet_require_signer_initialized())
    {
        return NULL;
    }

    waas_create_wallet_request_init(&params);
    waas_wallet_create_wallet_request_init(&request);
    waas_wallet_create_wallet_response_init(&response);
    oms_wallet_rpc_context_init(&rpc);

    if (oms_wallet_parse_wallet_type(wallet_type, &params.type, &rpc.error) != 0)
    {
        goto cleanup;
    }
    request.create_wallet_request = &params;

    if (oms_wallet_rpc_execute(
            &rpc,
            &request,
            &response,
            oms_wallet_create_wallet_prepare_request,
            oms_wallet_create_wallet_parse_response) != 0)
    {
        goto cleanup;
    }

    if (oms_wallet_finalize_wallet_response(
            &rpc,
            &wallet,
            response.create_wallet_response ? &response.create_wallet_response->wallet : NULL,
            "failed to extract CreateWallet response") != 0)
    {
        goto cleanup;
    }

cleanup:
    if (!wallet)
    {
        oms_wallet_log_waas_error("CreateWallet", &rpc.error);
    }

    oms_wallet_rpc_context_free(&rpc);
    waas_wallet_create_wallet_response_free(&response);
    waas_create_wallet_request_free(&params);
    return wallet;
}

oms_wallet_sign_message_response_t *oms_wallet_sign_message(
    const char *chain_id,
    const char *message)
{
    waas_sign_message_request params;
    waas_wallet_sign_message_request request;
    waas_wallet_sign_message_response *response = NULL;
    oms_wallet_rpc_context rpc;
    char *address = NULL;
    const char *network = NULL;

    if (!oms_wallet_require_signer_initialized())
    {
        return NULL;
    }

    waas_sign_message_request_init(&params);
    waas_wallet_sign_message_request_init(&request);
    oms_wallet_rpc_context_init(&rpc);

    response = oms_wallet_alloc_rpc_response(
        sizeof(*response),
        (void (*)(void *))waas_wallet_sign_message_response_init,
        &rpc,
        "failed to allocate SignMessage response");
    if (!response)
    {
        goto cleanup;
    }

    if (oms_wallet_prepare_wallet_target_params(
            chain_id,
            &params.network,
            &params.wallet_id,
            &rpc.error,
            "failed to prepare SignMessage request target") != 0)
    {
        goto cleanup;
    }
    params.message = waas_strdup(message);
    if (message && !params.message)
    {
        oms_wallet_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to prepare SignMessage request",
            NULL);
        goto cleanup;
    }
    request.sign_message_request = &params;

    if (oms_wallet_rpc_execute(
            &rpc,
            &request,
            response,
            oms_wallet_sign_message_prepare_request,
            oms_wallet_sign_message_parse_response) != 0)
    {
        goto cleanup;
    }

cleanup:
    waas_sign_message_request_free(&params);
    response = oms_wallet_finish_rpc_response(
        response,
        (void (*)(void *))waas_wallet_sign_message_response_free,
        &rpc,
        "SignMessage");
    oms_wallet_rpc_context_free(&rpc);
    return response;
}

oms_wallet_send_transaction_response_t *oms_wallet_send_transaction(
    const char *chain_id,
    const char *to,
    const char *value)
{
    waas_send_transaction_request params;
    waas_wallet_send_transaction_request request;
    waas_wallet_send_transaction_response *response = NULL;
    oms_wallet_rpc_context rpc;

    if (!oms_wallet_require_signer_initialized())
    {
        return NULL;
    }

    waas_send_transaction_request_init(&params);
    waas_wallet_send_transaction_request_init(&request);
    oms_wallet_rpc_context_init(&rpc);

    response = oms_wallet_alloc_rpc_response(
        sizeof(*response),
        (void (*)(void *))waas_wallet_send_transaction_response_init,
        &rpc,
        "failed to allocate SendTransaction response");
    if (!response)
    {
        goto cleanup;
    }

    if (oms_wallet_prepare_wallet_target_params(
            chain_id,
            &params.network,
            &params.wallet_id,
            &rpc.error,
            "failed to prepare SendTransaction request target") != 0)
    {
        goto cleanup;
    }
    params.to = waas_strdup(to);
    params.value = waas_strdup(value);
    params.mode = WAAS_TRANSACTION_MODE_RELAYER;
    if ((to && !params.to) ||
        (value && !params.value))
    {
        oms_wallet_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to prepare SendTransaction request",
            NULL);
        goto cleanup;
    }
    request.send_transaction_request = &params;

    if (oms_wallet_rpc_execute(
            &rpc,
            &request,
            response,
            oms_wallet_send_transaction_prepare_request,
            oms_wallet_send_transaction_parse_response) != 0)
    {
        goto cleanup;
    }

    if (!response->send_transaction_response || !response->send_transaction_response->tx_hash)
    {
        oms_wallet_set_waas_error(
            &rpc.error,
            "ClientError",
            "missing SendTransaction response payload",
            NULL);
        goto cleanup;
    }

cleanup:
    waas_send_transaction_request_free(&params);
    response = oms_wallet_finish_rpc_response(
        response,
        (void (*)(void *))waas_wallet_send_transaction_response_free,
        &rpc,
        "SendTransaction");
    oms_wallet_rpc_context_free(&rpc);
    return response;
}

void oms_wallet_free(oms_wallet_t *wallet)
{
    if (!wallet)
    {
        return;
    }

    waas_wallet_free(wallet);
    free(wallet);
}

void oms_wallet_free_complete_auth(oms_wallet_complete_auth_response_t *response)
{
    if (!response)
    {
        return;
    }

    waas_wallet_complete_auth_response_free(response);
    free(response);
}

void oms_wallet_free_sign_message(oms_wallet_sign_message_response_t *response)
{
    if (!response)
    {
        return;
    }

    waas_wallet_sign_message_response_free(response);
    free(response);
}

void oms_wallet_free_send_transaction(oms_wallet_send_transaction_response_t *response)
{
    if (!response)
    {
        return;
    }

    waas_wallet_send_transaction_response_free(response);
    free(response);
}
