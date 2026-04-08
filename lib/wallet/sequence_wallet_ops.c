#include "sequence_connector.h"
#include "sequence_wallet_internal.h"

#include <stdio.h>
#include <stdlib.h>

#include "chains/chain_bindings.h"
#include "storage/secure_storage.h"

waas_wallet *sequence_use_wallet(const char* wallet_type)
{
    waas_use_wallet_params params;
    waas_use_wallet_request request;
    waas_use_wallet_response response;
    sequence_wallet_rpc_context rpc;
    waas_wallet *wallet = NULL;

    if (!sequence_require_signer_initialized())
    {
        return NULL;
    }

    waas_use_wallet_params_init(&params);
    waas_use_wallet_request_init(&request);
    waas_use_wallet_response_init(&response);
    sequence_wallet_rpc_context_init(&rpc);

    if (sequence_parse_wallet_type(wallet_type, &params.wallet_type, &rpc.error) != 0)
    {
        goto cleanup;
    }
    params.wallet_index = 0;
    request.params = &params;

    if (sequence_wallet_rpc_execute(
            &rpc,
            &request,
            &response,
            sequence_use_wallet_prepare_request,
            sequence_use_wallet_parse_response) != 0)
    {
        goto cleanup;
    }

    if (sequence_finalize_wallet_response(
            &rpc,
            &wallet,
            &response.wallet,
            "failed to extract UseWallet response") != 0)
    {
        goto cleanup;
    }

cleanup:
    if (!wallet)
    {
        sequence_log_waas_error("UseWallet", &rpc.error);
    }

    sequence_wallet_rpc_context_free(&rpc);
    waas_use_wallet_response_free(&response);
    waas_use_wallet_params_free(&params);
    return wallet;
}

waas_wallet *sequence_create_wallet()
{
    return sequence_create_wallet_of_type(sequence_default_wallet_type());
}

waas_wallet *sequence_create_wallet_of_type(const char* walletType)
{
    waas_create_wallet_params params;
    waas_create_wallet_request request;
    waas_create_wallet_response response;
    sequence_wallet_rpc_context rpc;
    waas_wallet *wallet = NULL;

    if (!walletType)
    {
        walletType = sequence_default_wallet_type();
    }

    if (!sequence_require_signer_initialized())
    {
        return NULL;
    }

    waas_create_wallet_params_init(&params);
    waas_create_wallet_request_init(&request);
    waas_create_wallet_response_init(&response);
    sequence_wallet_rpc_context_init(&rpc);

    if (sequence_parse_wallet_type(walletType, &params.wallet_type, &rpc.error) != 0)
    {
        goto cleanup;
    }
    request.params = &params;

    if (sequence_wallet_rpc_execute(
            &rpc,
            &request,
            &response,
            sequence_create_wallet_prepare_request,
            sequence_create_wallet_parse_response) != 0)
    {
        goto cleanup;
    }

    if (sequence_finalize_wallet_response(
            &rpc,
            &wallet,
            &response.wallet,
            "failed to extract CreateWallet response") != 0)
    {
        goto cleanup;
    }

cleanup:
    if (!wallet)
    {
        sequence_log_waas_error("CreateWallet", &rpc.error);
    }

    sequence_wallet_rpc_context_free(&rpc);
    waas_create_wallet_response_free(&response);
    waas_create_wallet_params_free(&params);
    return wallet;
}

waas_sign_message_response *sequence_sign_message(
    const char* chain_id,
    const char* message)
{
    waas_sign_message_params params;
    waas_sign_message_request request;
    waas_sign_message_response *response = NULL;
    sequence_wallet_rpc_context rpc;
    char *address = NULL;
    const char *network;

    if (!sequence_require_signer_initialized())
    {
        return NULL;
    }

    waas_sign_message_params_init(&params);
    waas_sign_message_request_init(&request);
    sequence_wallet_rpc_context_init(&rpc);

    response = calloc(1, sizeof(*response));
    if (!response)
    {
        sequence_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to allocate SignMessage response",
            NULL);
        goto cleanup;
    }
    waas_sign_message_response_init(response);

    if (sequence_prepare_wallet_target_params(
            chain_id,
            &address,
            &network,
            &params.network,
            &params.wallet,
            &rpc.error,
            "failed to prepare SignMessage request target") != 0)
    {
        goto cleanup;
    }
    params.message = waas_strdup(message);
    if (message && !params.message)
    {
        sequence_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to prepare SignMessage request",
            NULL);
        goto cleanup;
    }
    request.params = &params;

    if (sequence_wallet_rpc_execute(
            &rpc,
            &request,
            response,
            sequence_sign_message_prepare_request,
            sequence_sign_message_parse_response) != 0)
    {
        goto cleanup;
    }

cleanup:
    if (!response || rpc.error.message)
    {
        sequence_log_waas_error("SignMessage", &rpc.error);
    }

    sequence_wallet_rpc_context_free(&rpc);
    free(address);
    waas_sign_message_params_free(&params);
    if (rpc.error.message)
    {
        if (response)
        {
            waas_sign_message_response_free(response);
            free(response);
        }
        return NULL;
    }
    return response;
}

waas_send_transaction_response *sequence_send_transaction(
    const char* chain_id,
    const char* to,
    const char* value)
{
    waas_send_transaction_params params;
    waas_send_transaction_request request;
    waas_send_transaction_response *response = NULL;
    sequence_wallet_rpc_context rpc;
    char *address = NULL;
    const char *network;

    if (!sequence_require_signer_initialized())
    {
        return NULL;
    }

    waas_send_transaction_params_init(&params);
    waas_send_transaction_request_init(&request);
    sequence_wallet_rpc_context_init(&rpc);

    response = calloc(1, sizeof(*response));
    if (!response)
    {
        sequence_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to allocate SendTransaction response",
            NULL);
        goto cleanup;
    }
    waas_send_transaction_response_init(response);

    if (sequence_prepare_wallet_target_params(
            chain_id,
            &address,
            &network,
            &params.network,
            &params.wallet,
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
        sequence_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to prepare SendTransaction request",
            NULL);
        goto cleanup;
    }
    request.params = &params;

    if (sequence_wallet_rpc_execute(
            &rpc,
            &request,
            response,
            sequence_send_transaction_prepare_request,
            sequence_send_transaction_parse_response) != 0)
    {
        goto cleanup;
    }

    if (!response->response || !response->response->tx_hash)
    {
        sequence_set_waas_error(
            &rpc.error,
            "ClientError",
            "missing SendTransaction response payload",
            NULL);
        goto cleanup;
    }

cleanup:
    if (!response || rpc.error.message)
    {
        sequence_log_waas_error("SendTransaction", &rpc.error);
    }

    sequence_wallet_rpc_context_free(&rpc);
    free(address);
    waas_send_transaction_params_free(&params);
    if (rpc.error.message)
    {
        if (response)
        {
            waas_send_transaction_response_free(response);
            free(response);
        }
        return NULL;
    }
    return response;
}
