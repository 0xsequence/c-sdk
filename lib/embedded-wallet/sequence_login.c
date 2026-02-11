#include "sequence_login.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "evm/eoa_wallet.h"
#include "evm/sign_message.h"
#include "requests/build_intent_json.h"
#include "requests/build_initiate_auth_intent_json.h"
#include "requests/build_open_session_intent_json.h"
#include "utils/globals.h"
#include "../utils/timestamps.h"
#include "evm/keccak256.h"
#include "networking/http_client.h"
#include "requests/build_signable_intent_json.h"
#include "requests/initiate_auth_intent_return.h"
#include "requests/open_session_intent_return.h"
#include "utils/byte_utils.h"
#include "utils/hex_utils.h"
#include "utils/string_utils.h"

static eoa_wallet_t *cur_signer = NULL;
static char *cur_challenge = NULL;

int sign_in_with_email(const char *email) {
    HttpClient *c = http_client_create(g_wallet_api_url);
    if (!c) {
        fprintf(stderr, "Failed to create HttpClient\n");
        return -1;
    }

    http_add_sequence_access_key(c);
    http_client_add_header(c, "Accept: application/json");

    cur_signer = calloc(1, sizeof(*cur_signer)); // sizeof(eoa_wallet_t)
    if (!cur_signer) return -1;

    if (eoa_wallet_initialize(cur_signer) == 0) {
        free(cur_signer);
        cur_signer = NULL;
        return -1;
    }

    const char *address = eoa_wallet_get_address(
        cur_signer->ctx,
        &cur_signer->pubkey);

    size_t addr_len;
    uint8_t *addr = hex_to_bytes(
        address,
        &addr_len
    );

    size_t new_len;
    uint8_t *padded = prepend_zero(addr, 20, &new_len);

    char *session_id = bytes_to_hex(padded, new_len);

    printf("Session ID: %s\n", session_id);

    cJSON *intent_data = sequence_build_initiate_auth_intent_json(
      email,
      "",
      session_id
    );

    if (!intent_data)
    {
        free(cur_signer);
        cur_signer = NULL;
        fprintf(stderr, "Failed to create intent_data\n");
        return -1;
    }

    long issuedAt = timestamp_now_seconds();
    long expiresAt = timestamp_seconds_from_now(36000);

    char *to_sign = build_signable_intent_json(
        intent_data,
        "initiateAuth",
        issuedAt,
        expiresAt
    );

    uint8_t hashed_to_sign[32];
    keccak256((const uint8_t*)to_sign, strlen(to_sign), hashed_to_sign);

    const char *sig = wallet_sign_message_hex_eip191(
        cur_signer->seckey,
        cur_signer->ctx,
        hashed_to_sign,
        sizeof(hashed_to_sign),
        NULL, 0);

    cJSON *intent_data_2 = sequence_build_initiate_auth_intent_json(
          email,
          "",
          session_id);

    if (!intent_data_2)
    {
        free(cur_signer);
        cur_signer = NULL;
        fprintf(stderr, "Failed to create intent_data\n");
        return -1;
    }

    char *intent_json = sequence_build_intent_json(
        intent_data_2,
        "initiateAuth",
        issuedAt,
        expiresAt,
        session_id,
        sig
    );

    printf(">> %s", intent_json);

    HttpResponse r = http_client_post_json(c, "/CommitVerifier", intent_json, 10000);

    if (r.error) {
        fprintf(stderr, "Request failed: %s\n", r.error);
        free(cur_signer);
        cur_signer = NULL;
        http_response_free(&r);
        http_client_destroy(c);
        return -1;
    }

    printf("Response: %s\n", r.body);

    SequenceInitiateAuthResponse response = sequence_build_initiate_auth_intent_return(r.body);
    cur_challenge = strdup(response.data.challenge);

    http_response_free(&r);
    http_client_destroy(c);

    return 1;
}

sequence_wallet_t *confirm_email_sign_in(const char *email, const char *code) {
    if (!cur_signer || !cur_signer->ctx) {
        fprintf(stderr, "No signer initialized\n");
        return NULL;
    }
    if (!cur_challenge) {
        fprintf(stderr, "No challenge available\n");
        free(cur_signer);
        cur_signer = NULL;
        return NULL;
    }

    HttpClient *c = http_client_create(g_wallet_api_url);
    if (!c) {
        fprintf(stderr, "Failed to create HttpClient\n");
        free(cur_signer);
        cur_signer = NULL;
        return NULL;
    }

    http_add_sequence_access_key(c);
    http_client_add_header(c, "Accept: application/json");

    const char *address = eoa_wallet_get_address(cur_signer->ctx, &cur_signer->pubkey);

    size_t addr_len;
    uint8_t *addr = hex_to_bytes(
        address,
        &addr_len
    );

    size_t new_len;
    uint8_t *padded = prepend_zero(addr, 20, &new_len);

    char *session_id = bytes_to_hex(padded, new_len);

    printf("Session ID: %s, %s, %s\n", session_id, cur_challenge, code);

    long issuedAt = timestamp_now_seconds();
    long expiresAt = timestamp_seconds_from_now(36000);

    char *preHashAnswer = concat_malloc(cur_challenge, code);

    uint8_t hashedAnswer[32];
    keccak256((const uint8_t*)preHashAnswer, strlen(preHashAnswer), hashedAnswer);

    char *hashedAnswerHex = bytes_to_hex(hashedAnswer, 32);

    char *to_sign = build_signable_intent_json(
        sequence_build_open_session_intent_json(
          email,
          session_id,
          hashedAnswerHex
        ),
        "openSession",
        issuedAt,
        expiresAt
    );

    uint8_t hashed_to_sign[32];
    keccak256((const uint8_t*)to_sign, strlen(to_sign), hashed_to_sign);

    const char *sig = wallet_sign_message_hex_eip191(
        cur_signer->seckey,
        cur_signer->ctx,
        hashed_to_sign,
        sizeof(hashed_to_sign),
        NULL, 0);

    char *intent_json = sequence_build_intent_json(
        sequence_build_open_session_intent_json(
          email,
          session_id,
          hashedAnswerHex
        ),
        "openSession",
        issuedAt,
        expiresAt,
        session_id,
        sig
    );

	printf(">> %s", intent_json);

    HttpResponse r = http_client_post_json(c, "/CompleteAuth", intent_json, 10000);

    if (r.error) {
        fprintf(stderr, "Request failed: %s\n", r.error);
        free(cur_signer);
        cur_signer = NULL;
        http_response_free(&r);
        http_client_destroy(c);
        return NULL;
    }

    printf("Response: %s\n", r.body);

    SequenceSessionOpenedResult sessionData = sequence_build_open_session_intent_return(r.body);

    const char *new_session_id = sessionData.responseData.sessionId;
    const char *wallet_address = sessionData.responseData.wallet;

    sequence_wallet_t *sequence_wallet = sequence_wallet_from_response(
        wallet_address,
        email,
        new_session_id,
        cur_signer->seckey);

    http_response_free(&r);
    http_client_destroy(c);

    return sequence_wallet;
}
