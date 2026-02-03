#include "sequence_login.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "evm/sign_message.h"
#include "requests/build_intent_json.h"
#include "requests/build_initiate_auth_intent_json.h"
#include "requests/build_open_session_intent_json.h"
#include "../utils/timestamps.h"
#include "evm/keccak256.h"
#include "networking/http_client.h"
#include "requests/build_signable_intent_json.h"
#include "requests/initiate_auth_intent_return.h"
#include "requests/open_session_intent_return.h"
#include "utils/byte_utils.h"
#include "utils/hex_utils.h"
#include "utils/string_utils.h"

static sequence_wallet_t *cur_signer = NULL;
static char *cur_challenge = NULL;

int sign_in_with_email(const char *email) {
    HttpClient *c = http_client_create("https://waas.sequence.app/rpc/WaasAuthenticator");
    if (!c) {
        fprintf(stderr, "Failed to create HttpClient\n");
        return 1;
    }

    http_add_sequence_access_key(c);
    http_client_add_header(c, "Accept: application/json");

    cur_signer = (sequence_wallet_t *)calloc(1, sizeof(sequence_wallet_t));
    if (!cur_signer) return -1;

    if (!sequence_wallet_initialize(cur_signer)) {
        free(cur_signer);
        return -1;
    }

    const char *address = sequence_wallet_get_address(cur_signer->ctx, &cur_signer->pubkey);

    size_t addr_len;
    uint8_t *addr = hex_to_bytes(
        address,
        &addr_len
    );

    size_t new_len;
    uint8_t *padded = prepend_zero(addr, 20, &new_len);

    char *session_id = bytes_to_hex(padded, new_len);

    printf("Public address: %s\n", session_id);

    cJSON *intent_data = sequence_build_initiate_auth_intent_json(
      email,
      "",
      session_id
    );

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

    char *hashed_to_sign_hex = bytes_to_hex(hashed_to_sign, sizeof(hashed_to_sign));

    printf("data ready to sign: %s\n", to_sign);
    printf("hashed data: %s\n", hashed_to_sign_hex);

    const char *sig = wallet_sign_message_hex_eip191(cur_signer->seckey, cur_signer->ctx, hashed_to_sign, sizeof(hashed_to_sign), NULL, 0);

    printf("signature: %s\n", sig);

    char *intent_json = sequence_build_intent_json(
    sequence_build_initiate_auth_intent_json(
          email,
          "",
          session_id
        ),
        "initiateAuth",
        issuedAt,
        expiresAt,
        session_id,
        sig
    );

    printf(">> %s", intent_json);

    HttpResponse r = http_client_post_json(c, "/SendIntent", intent_json, 10000);

    if (r.error) {
        fprintf(stderr, "Request failed: %s\n", r.error);
        http_response_free(&r);
        http_client_destroy(c);
        return 1;
    }

    printf("Status: %ld\n", r.status_code);
    printf("Body (%zu bytes):\n%s\n", r.body_len, r.body);

    SequenceInitiateAuthResponse response = sequence_build_initiate_auth_intent_return(r.body);
    cur_challenge = response.data.challenge;

    http_response_free(&r);
    http_client_destroy(c);
    return 1;
}

sequence_wallet_t *confirm_email_sign_in(const char *email, const char *code) {
    HttpClient *c = http_client_create("https://waas.sequence.app/rpc/WaasAuthenticator");
    if (!c) {
        fprintf(stderr, "Failed to create HttpClient\n");
        return NULL;
    }

    http_add_sequence_access_key(c);
    http_client_add_header(c, "Accept: application/json");

    const char *address = sequence_wallet_get_address(cur_signer->ctx, &cur_signer->pubkey);

    size_t addr_len;
    uint8_t *addr = hex_to_bytes(
        address,
        &addr_len
    );

    size_t new_len;
    uint8_t *padded = prepend_zero(addr, 20, &new_len);

    char *session_id = bytes_to_hex(padded, new_len);

    printf("Public address: %s\n", session_id);

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

    char *hashed_to_sign_hex = bytes_to_hex(hashed_to_sign, sizeof(hashed_to_sign));

    printf("data ready to sign: %s\n", to_sign);
    printf("hashed data: %s\n", hashed_to_sign_hex);

    const char *sig = wallet_sign_message_hex_eip191(
        cur_signer->seckey,
        cur_signer->ctx,
        hashed_to_sign,
        sizeof(hashed_to_sign),
        NULL, 0);

    printf("signature: %s\n", sig);

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

    HttpResponse r = http_client_post_json(c, "/RegisterSession", intent_json, 10000);

    if (r.error) {
        fprintf(stderr, "Request failed: %s\n", r.error);
        http_response_free(&r);
        http_client_destroy(c);
        return NULL;
    }

    printf("Status: %ld\n", r.status_code);
    printf("Body (%zu bytes):\n%s\n", r.body_len, r.body);

    SequenceSessionOpenedResult sessionData = sequence_build_open_session_intent_return(r.body);

    char *new_session_id = sessionData.responseData.sessionId;
    char *walletAddress = sessionData.responseData.wallet;

    printf("wallet address: %s\n", walletAddress);
    printf("Session ID: %s\n", new_session_id);

    http_response_free(&r);
    http_client_destroy(c);

    return cur_signer;
}

void sequence_wallet_free(sequence_wallet_t *wallet) {
    if (!wallet) return;
    sequence_wallet_clear(wallet);
    free(wallet);
}
