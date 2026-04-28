#include "oms_wallet.h"
#include "oms_wallet_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/base64url.h"
#include "utils/sha256.h"
#include "utils/string_utils.h"

static int oms_wallet_clear_persisted_session_state(oms_wallet_sdk_t *sdk)
{
    int status = 0;
    int next_status = 0;
    char *stored_signer_id = NULL;

    if (!sdk)
    {
        return -1;
    }

    next_status = oms_wallet_session_read_string(sdk, "oms_auth_signer_id", &stored_signer_id);
    if (next_status == 0 && stored_signer_id && stored_signer_id[0] != '\0')
    {
        next_status = oms_wallet_auth_signer_delete(sdk, stored_signer_id);
        if (next_status != 0 && status == 0)
        {
            status = next_status;
        }
    }
    else if (next_status != 0 && !oms_wallet_session_status_is_not_found(sdk, next_status) && status == 0)
    {
        status = next_status;
    }
    oms_wallet_session_free_string(sdk, stored_signer_id);

    next_status = oms_wallet_session_delete(sdk, "challenge");
    if (next_status != 0 && !oms_wallet_session_status_is_not_found(sdk, next_status) && status == 0)
    {
        status = next_status;
    }

    next_status = oms_wallet_session_delete(sdk, "verifier");
    if (next_status != 0 && !oms_wallet_session_status_is_not_found(sdk, next_status) && status == 0)
    {
        status = next_status;
    }

    next_status = oms_wallet_session_delete(sdk, "oms_auth_signer_id");
    if (next_status != 0 && !oms_wallet_session_status_is_not_found(sdk, next_status) && status == 0)
    {
        status = next_status;
    }

    next_status = oms_wallet_session_delete(sdk, "oms_wallet_id");
    if (next_status != 0 && !oms_wallet_session_status_is_not_found(sdk, next_status) && status == 0)
    {
        status = next_status;
    }

    next_status = oms_wallet_session_delete(sdk, "oms_wallet_address");
    if (next_status != 0 && !oms_wallet_session_status_is_not_found(sdk, next_status) && status == 0)
    {
        status = next_status;
    }

    return status;
}

static int oms_wallet_persist_pending_sign_in_state(oms_wallet_sdk_t *sdk)
{
    int status;

    if (!sdk || !sdk->auth_signer_id)
    {
        return -1;
    }

    status = oms_wallet_session_write_string(sdk, "challenge", sdk->challenge);
    if (status != 0)
    {
        oms_wallet_clear_persisted_session_state(sdk);
        return status;
    }

    status = oms_wallet_session_write_string(sdk, "verifier", sdk->verifier);
    if (status != 0)
    {
        oms_wallet_clear_persisted_session_state(sdk);
        return status;
    }

    status = oms_wallet_session_write_string(sdk, "oms_auth_signer_id", sdk->auth_signer_id);
    if (status != 0)
    {
        oms_wallet_clear_persisted_session_state(sdk);
        return status;
    }

    return 0;
}

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

int oms_wallet_restore_session(oms_wallet_sdk_t *sdk)
{
    char *restored_signer_id = NULL;
    char *restored_wallet_id = NULL;
    char *restored_challenge = NULL;
    char *restored_verifier = NULL;
    char *key_type = NULL;
    char *credential = NULL;
    int challenge_status = 0;
    int verifier_status = 0;
    int status;

    if (!sdk)
    {
        return -1;
    }

    status = oms_wallet_session_read_string(sdk, "oms_auth_signer_id", &restored_signer_id);

    if (status != 0)
    {
        if (oms_wallet_session_status_is_not_found(sdk, status))
        {
            return 0;
        }

        fprintf(stderr, "Failed to read stored auth signer id (error=%d)\n", status);
        return -1;
    }

    if (!restored_signer_id || restored_signer_id[0] == '\0' ||
        oms_wallet_auth_signer_get_credential(sdk, restored_signer_id, &key_type, &credential) != 0)
    {
        oms_wallet_session_free_string(sdk, restored_signer_id);
        oms_wallet_auth_signer_free_string(sdk, key_type);
        oms_wallet_auth_signer_free_string(sdk, credential);
        fprintf(stderr, "Failed to restore auth signer from stored signer id\n");
        return -1;
    }
    oms_wallet_auth_signer_free_string(sdk, key_type);
    oms_wallet_auth_signer_free_string(sdk, credential);

    clear_current_signer(sdk);
    free(sdk->challenge);
    sdk->challenge = NULL;
    free(sdk->verifier);
    sdk->verifier = NULL;
    free(sdk->wallet_id);
    sdk->wallet_id = NULL;

    sdk->auth_signer_id = restored_signer_id;
    restored_signer_id = NULL;

    status = oms_wallet_session_read_string(sdk, "oms_wallet_id", &restored_wallet_id);
    if (status == 0 && restored_wallet_id && restored_wallet_id[0] != '\0')
    {
        sdk->wallet_id = restored_wallet_id;
        return 1;
    }
    if (status != 0 && !oms_wallet_session_status_is_not_found(sdk, status))
    {
        fprintf(stderr, "Failed to read stored wallet id (error=%d)\n", status);
        oms_wallet_session_free_string(sdk, restored_wallet_id);
        clear_current_signer(sdk);
        return -1;
    }

    oms_wallet_session_free_string(sdk, restored_wallet_id);
    challenge_status = oms_wallet_session_read_string(sdk, "challenge", &restored_challenge);
    if (challenge_status != 0 && !oms_wallet_session_status_is_not_found(sdk, challenge_status))
    {
        fprintf(stderr, "Failed to read stored challenge (error=%d)\n", challenge_status);
        clear_current_signer(sdk);
        return -1;
    }

    verifier_status = oms_wallet_session_read_string(sdk, "verifier", &restored_verifier);
    if (verifier_status != 0 && !oms_wallet_session_status_is_not_found(sdk, verifier_status))
    {
        fprintf(stderr, "Failed to read stored verifier (error=%d)\n", verifier_status);
        clear_current_signer(sdk);
        oms_wallet_session_free_string(sdk, restored_challenge);
        return -1;
    }

    if (challenge_status == 0 && verifier_status == 0 &&
        restored_challenge && restored_challenge[0] != '\0' &&
        restored_verifier && restored_verifier[0] != '\0')
    {
        sdk->challenge = restored_challenge;
        sdk->verifier = restored_verifier;
        return 1;
    }

    if (oms_wallet_session_status_is_not_found(sdk, challenge_status) &&
        oms_wallet_session_status_is_not_found(sdk, verifier_status))
    {
        clear_current_signer(sdk);
        return 0;
    }

    fprintf(stderr, "Stored sign-in session is incomplete\n");
    clear_current_signer(sdk);
    oms_wallet_session_free_string(sdk, restored_challenge);
    oms_wallet_session_free_string(sdk, restored_verifier);
    return -1;
}

int oms_wallet_start_email_sign_in(oms_wallet_sdk_t *sdk, const char *email)
{
    waas_commit_verifier_request params;
    waas_wallet_commit_verifier_request request;
    waas_wallet_commit_verifier_response response;
    oms_wallet_rpc_context rpc;
    int status = -1;
    int storage_status = 0;
    char storage_cause[32];

    if (!sdk)
    {
        return -1;
    }

    waas_commit_verifier_request_init(&params);
    waas_wallet_commit_verifier_request_init(&request);
    waas_wallet_commit_verifier_response_init(&response);
    oms_wallet_rpc_context_init(&rpc, sdk);

    clear_current_signer(sdk);
    free(sdk->challenge);
    sdk->challenge = NULL;
    free(sdk->verifier);
    sdk->verifier = NULL;
    free(sdk->wallet_id);
    sdk->wallet_id = NULL;

    storage_status = oms_wallet_clear_persisted_session_state(sdk);
    if (storage_status != 0)
    {
        snprintf(storage_cause, sizeof(storage_cause), "storage error=%d", storage_status);
        oms_wallet_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to clear prior sign-in session state",
            storage_cause);
        goto cleanup;
    }

    if (oms_wallet_auth_signer_create(sdk, &sdk->auth_signer_id) != 0 || !sdk->auth_signer_id)
    {
        clear_current_signer(sdk);
        oms_wallet_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to create auth signer",
            NULL);
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

    sdk->challenge = waas_strdup(response.commit_verifier_response->challenge);
    sdk->verifier = waas_strdup(response.commit_verifier_response->verifier);
    if (!sdk->challenge || !sdk->verifier)
    {
        oms_wallet_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to copy CommitVerifier response",
            NULL);
        goto cleanup;
    }

    storage_status = oms_wallet_persist_pending_sign_in_state(sdk);
    if (storage_status != 0)
    {
        snprintf(storage_cause, sizeof(storage_cause), "storage error=%d", storage_status);
        oms_wallet_set_waas_error(
            &rpc.error,
            "ClientError",
            "failed to persist sign-in session state",
            storage_cause);
        goto cleanup;
    }

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
        if (sdk->auth_signer_id)
        {
            oms_wallet_auth_signer_delete(sdk, sdk->auth_signer_id);
        }
        clear_current_signer(sdk);
        free(sdk->challenge);
        sdk->challenge = NULL;
        free(sdk->verifier);
        sdk->verifier = NULL;
    }

    oms_wallet_rpc_context_free(&rpc);
    waas_wallet_commit_verifier_response_free(&response);
    waas_commit_verifier_request_free(&params);
    return status;
}

waas_wallet_complete_auth_response *oms_wallet_complete_email_sign_in(
    oms_wallet_sdk_t *sdk,
    const char *code)
{
    waas_complete_auth_request params;
    waas_wallet_complete_auth_request request;
    waas_wallet_complete_auth_response *response = NULL;
    oms_wallet_rpc_context rpc;
    char *hashed_answer = NULL;
    int status = -1;

    if (!oms_wallet_require_signer_initialized(sdk))
    {
        return NULL;
    }

    if (!sdk->challenge)
    {
        fprintf(stderr, "No challenge available\n");
        clear_current_signer(sdk);
        return NULL;
    }

    if (!sdk->verifier)
    {
        fprintf(stderr, "No verifier available\n");
        clear_current_signer(sdk);
        return NULL;
    }

    waas_complete_auth_request_init(&params);
    waas_wallet_complete_auth_request_init(&request);
    oms_wallet_rpc_context_init(&rpc, sdk);

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

    hashed_answer = oms_wallet_hash_auth_answer(sdk->challenge, code);

    params.identity_type = WAAS_IDENTITY_TYPE_EMAIL;
    params.auth_mode = WAAS_AUTH_MODE_OTP;
    params.verifier = waas_strdup(sdk->verifier);
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
