#include "sequence_connector.h"
#include "sequence_wallet_internal.h"

#include <stdio.h>
#include <stdlib.h>

#include "evm/keccak256.h"
#include "storage/secure_storage.h"
#include "utils/hex_utils.h"
#include "utils/string_utils.h"

int sequence_restore_session()
{
    uint8_t seckey[32];
    eoa_wallet_t *restored_signer;
    int status = secure_store_read_seckey(seckey);

    if (status != 0)
    {
        printf("Failed to read seckey (error=%d)\n", status);
        return -1;
    }

    restored_signer = calloc(1, sizeof(*restored_signer));
    if (!restored_signer)
    {
        fprintf(stderr, "Failed to allocate signer while restoring session\n");
        return -1;
    }

    if (!eoa_wallet_from_private_key_bytes(restored_signer, seckey))
    {
        free(restored_signer);
        fprintf(stderr, "Failed to rebuild signer from stored private key\n");
        return -1;
    }

    clear_current_signer();
    free(cur_challenge);
    cur_challenge = NULL;
    free(cur_verifier);
    cur_verifier = NULL;

    cur_signer = restored_signer;

    secure_store_read_string("challenge", &cur_challenge);
    secure_store_read_string("verifier", &cur_verifier);
    return 1;
}

int sequence_sign_in_with_email(const char* email)
{
    waas_commit_verifier_request params;
    waas_wallet_commit_verifier_request request;
    waas_wallet_commit_verifier_response response;
    sequence_wallet_rpc_context rpc;
    int status = -1;

    waas_commit_verifier_request_init(&params);
    waas_wallet_commit_verifier_request_init(&request);
    waas_wallet_commit_verifier_response_init(&response);
    sequence_wallet_rpc_context_init(&rpc);

    clear_current_signer();
    free(cur_challenge);
    cur_challenge = NULL;
    free(cur_verifier);
    cur_verifier = NULL;

    cur_signer = calloc(1, sizeof(*cur_signer));
    if (!cur_signer)
    {
        goto cleanup;
    }

    if (eoa_wallet_initialize(cur_signer) == 0)
    {
        clear_current_signer();
        goto cleanup;
    }

    params.identity_type = WAAS_IDENTITY_TYPE_EMAIL;
    params.auth_mode = WAAS_AUTH_MODE_OTP;
    params.has_handle = true;
    params.handle = waas_strdup(email);
    if (!params.handle)
    {
        sequence_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to allocate CommitVerifier handle",
            NULL);
        goto cleanup;
    }
    request.commit_verifier_request = &params;

    if (sequence_wallet_rpc_execute(
            &rpc,
            &request,
            &response,
            sequence_commit_verifier_prepare_request,
            sequence_commit_verifier_parse_response) != 0)
    {
        goto cleanup;
    }

    if (!response.commit_verifier_response)
    {
        sequence_set_waas_error(
            &rpc.error,
            "ClientError",
            "CommitVerifier response missing payload",
            NULL);
        goto cleanup;
    }

    cur_challenge = waas_strdup(response.commit_verifier_response->challenge);
    cur_verifier = waas_strdup(response.commit_verifier_response->verifier);
    if (!cur_challenge || !cur_verifier)
    {
        sequence_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to copy CommitVerifier response",
            NULL);
        goto cleanup;
    }

    secure_store_write_string("challenge", cur_challenge);
    secure_store_write_string("verifier", cur_verifier);
    secure_store_write_seckey(cur_signer->seckey);

    status = 1;

cleanup:
    if (status != 1)
    {
        if (rpc.error.message)
        {
            fprintf(
                stderr,
                "CommitVerifier failed: %s%s%s%s\n",
                rpc.error.message,
                rpc.error.cause ? " (" : "",
                rpc.error.cause ? rpc.error.cause : "",
                rpc.error.cause ? ")" : "");
        }
        clear_current_signer();
        free(cur_challenge);
        cur_challenge = NULL;
        free(cur_verifier);
        cur_verifier = NULL;
    }

    sequence_wallet_rpc_context_free(&rpc);
    waas_wallet_commit_verifier_response_free(&response);
    waas_commit_verifier_request_free(&params);
    return status;
}

waas_wallet_complete_auth_response *sequence_confirm_email_sign_in(
    const char* code)
{
    waas_complete_auth_request params;
    waas_wallet_complete_auth_request request;
    waas_wallet_complete_auth_response *response = NULL;
    sequence_wallet_rpc_context rpc;
    char *pre_hash_answer = NULL;
    char *hashed_answer_hex = NULL;
    int status = -1;

    if (!sequence_require_signer_initialized())
    {
        return NULL;
    }

    if (!cur_challenge)
    {
        fprintf(stderr, "No challenge available\n");
        clear_current_signer();
        return NULL;
    }

    if (!cur_verifier)
    {
        fprintf(stderr, "No verifier available\n");
        clear_current_signer();
        return NULL;
    }

    waas_complete_auth_request_init(&params);
    waas_wallet_complete_auth_request_init(&request);
    sequence_wallet_rpc_context_init(&rpc);

    response = calloc(1, sizeof(*response));
    if (!response)
    {
        sequence_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to allocate CompleteAuth response",
            NULL);
        goto cleanup;
    }
    waas_wallet_complete_auth_response_init(response);

    pre_hash_answer = concat_malloc(cur_challenge, code);
    if (!pre_hash_answer)
    {
        sequence_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to allocate CompleteAuth challenge answer",
            NULL);
        goto cleanup;
    }

    {
        uint8_t hashed_to_sign[32];
        keccak256((const uint8_t*)pre_hash_answer, strlen(pre_hash_answer), hashed_to_sign);
        hashed_answer_hex = bytes_to_hex(hashed_to_sign, 32);
    }

    params.identity_type = WAAS_IDENTITY_TYPE_EMAIL;
    params.auth_mode = WAAS_AUTH_MODE_OTP;
    params.verifier = waas_strdup(cur_verifier);
    params.answer = waas_strdup(hashed_answer_hex);
    if (!hashed_answer_hex || !params.verifier || !params.answer)
    {
        sequence_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to prepare CompleteAuth request",
            NULL);
        goto cleanup;
    }
    request.complete_auth_request = &params;

    if (sequence_wallet_rpc_execute(
            &rpc,
            &request,
            response,
            sequence_complete_auth_prepare_request,
            sequence_complete_auth_parse_response) != 0)
    {
        goto cleanup;
    }

cleanup:
    if (!response || rpc.error.message)
    {
        sequence_log_waas_error("CompleteAuth", &rpc.error);
    }

    status = rpc.error.message ? -1 : 0;
    free(hashed_answer_hex);
    free(pre_hash_answer);
    waas_complete_auth_request_free(&params);
    if (status != 0)
    {
        if (response)
        {
            waas_wallet_complete_auth_response_free(response);
            free(response);
        }
        response = NULL;
    }
    sequence_wallet_rpc_context_free(&rpc);
    return response;
}
