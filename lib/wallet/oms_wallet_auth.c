#include "oms_wallet.h"
#include "oms_wallet_internal.h"

#include <stdio.h>
#include <stdlib.h>

#include "storage/secure_storage.h"
#include "utils/base64url.h"
#include "utils/sha256.h"
#include "utils/string_utils.h"

static char *oms_wallet_hash_auth_answer(const char *challenge, const char *answer)
{
    char *preimage;
    uint8_t digest[32];
    char *encoded;

    if (!challenge || !answer)
    {
        return NULL;
    }

    preimage = concat_malloc(challenge, answer);
    if (!preimage)
    {
        return NULL;
    }

    oms_wallet_sha256((const uint8_t *)preimage, strlen(preimage), digest);
    encoded = oms_wallet_base64url_encode_unpadded(digest, sizeof(digest));
    free(preimage);
    return encoded;
}

int oms_wallet_restore_session()
{
    uint8_t seckey[32];
    eoa_wallet_t *restored_signer;
    char *restored_wallet_id = NULL;
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
    free(cur_wallet_id);
    cur_wallet_id = NULL;

    cur_signer = restored_signer;

    secure_store_read_string("oms_wallet_id", &restored_wallet_id);
    if (restored_wallet_id && restored_wallet_id[0] != '\0')
    {
        cur_wallet_id = restored_wallet_id;
        return 1;
    }

    free(restored_wallet_id);
    secure_store_read_string("challenge", &cur_challenge);
    secure_store_read_string("verifier", &cur_verifier);
    return 1;
}

int oms_wallet_start_email_sign_in(const char *email)
{
    waas_commit_verifier_request params;
    waas_wallet_commit_verifier_request request;
    waas_wallet_commit_verifier_response response;
    oms_wallet_rpc_context rpc;
    int status = -1;

    waas_commit_verifier_request_init(&params);
    waas_wallet_commit_verifier_request_init(&request);
    waas_wallet_commit_verifier_response_init(&response);
    oms_wallet_rpc_context_init(&rpc);

    clear_current_signer();
    free(cur_challenge);
    cur_challenge = NULL;
    free(cur_verifier);
    cur_verifier = NULL;
    free(cur_wallet_id);
    cur_wallet_id = NULL;

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
        oms_wallet_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to allocate CommitVerifier handle",
            NULL);
        goto cleanup;
    }
    request.commit_verifier_request = &params;

    if (oms_wallet_rpc_execute(
            &rpc,
            &request,
            &response,
            oms_wallet_commit_verifier_prepare_request,
            oms_wallet_commit_verifier_parse_response) != 0)
    {
        goto cleanup;
    }

    if (!response.commit_verifier_response)
    {
        oms_wallet_set_waas_error(
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
        oms_wallet_set_waas_error(
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

    oms_wallet_rpc_context_free(&rpc);
    waas_wallet_commit_verifier_response_free(&response);
    waas_commit_verifier_request_free(&params);
    return status;
}

waas_wallet_complete_auth_response *oms_wallet_complete_email_sign_in(
    const char *code)
{
    waas_complete_auth_request params;
    waas_wallet_complete_auth_request request;
    waas_wallet_complete_auth_response *response = NULL;
    oms_wallet_rpc_context rpc;
    char *hashed_answer = NULL;
    int status = -1;

    if (!oms_wallet_require_signer_initialized())
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
    oms_wallet_rpc_context_init(&rpc);

    response = calloc(1, sizeof(*response));
    if (!response)
    {
        oms_wallet_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to allocate CompleteAuth response",
            NULL);
        goto cleanup;
    }
    waas_wallet_complete_auth_response_init(response);

    hashed_answer = oms_wallet_hash_auth_answer(cur_challenge, code);

    params.identity_type = WAAS_IDENTITY_TYPE_EMAIL;
    params.auth_mode = WAAS_AUTH_MODE_OTP;
    params.verifier = waas_strdup(cur_verifier);
    params.answer = waas_strdup(hashed_answer);
    if (!hashed_answer || !params.verifier || !params.answer)
    {
        oms_wallet_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to prepare CompleteAuth request",
            NULL);
        goto cleanup;
    }
    request.complete_auth_request = &params;

    if (oms_wallet_rpc_execute(
            &rpc,
            &request,
            response,
            oms_wallet_complete_auth_prepare_request,
            oms_wallet_complete_auth_parse_response) != 0)
    {
        goto cleanup;
    }

cleanup:
    if (!response || rpc.error.message)
    {
        oms_wallet_log_waas_error("CompleteAuth", &rpc.error);
    }

    status = rpc.error.message ? -1 : 0;
    free(hashed_answer);
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
    oms_wallet_rpc_context_free(&rpc);
    return response;
}
