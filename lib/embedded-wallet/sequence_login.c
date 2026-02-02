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
#include "utils/byte_utils.h"
#include "utils/hex_utils.h"

static int is_reasonable_email(const char *email) {
    if (!email) return 0;
    size_t n = strlen(email);
    if (n < 3 || n > 254) return 0;

    const char *at = strchr(email, '@');
    if (!at) return 0;
    if (at == email) return 0;                 /* nothing before @ */
    if (*(at + 1) == '\0') return 0;           /* nothing after @ */
    if (strchr(at + 1, '.') == NULL) return 0; /* require dot after @ */

    /* reject whitespace */
    for (const char *p = email; *p; p++) {
        if (isspace((unsigned char)*p)) return 0;
    }
    return 1;
}

static int is_reasonable_code(const char *code) {
    if (!code) return 0;
    size_t n = strlen(code);
    if (n < 4 || n > 12) return 0; /* flexible: 4-12 chars */
    for (size_t i = 0; i < n; i++) {
        if (!isalnum((unsigned char)code[i])) return 0;
    }
    return 1;
}

int sign_in_with_email(const char *email) {
    if (!is_reasonable_email(email)) {
        fprintf(stderr, "sign_in_with_email: invalid email\n");
        return 0;
    }

    HttpClient *c = http_client_create("https://waas.sequence.app/rpc/WaasAuthenticator");
    if (!c) {
        fprintf(stderr, "Failed to create HttpClient\n");
        return 1;
    }

    http_add_sequence_access_key(c);
    http_client_add_header(c, "Accept: application/json");

    sequence_wallet_t *wallet = (sequence_wallet_t *)calloc(1, sizeof(sequence_wallet_t));
    if (!wallet) return -1;

    if (!sequence_wallet_initialize(wallet)) {
        free(wallet);
        return -1;
    }

    const char *address = sequence_wallet_get_address(wallet->ctx, &wallet->pubkey);

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

    char *to_sign = cJSON_PrintUnformatted(intent_data);

    uint8_t bytes[32];

    size_t len = string_to_bytes(to_sign, bytes, sizeof(bytes));

    printf("Bytes for to_sign (%zu):\n", len);
    for (size_t i = 0; i < len; i++) {
        printf("0x%02X ", bytes[i]);
    }
    printf("\n");

    uint8_t hashed_to_sign[32];
    keccak256(bytes, sizeof(bytes), hashed_to_sign);

    char *hashed_to_sign_hex = bytes_to_hex(hashed_to_sign, sizeof(hashed_to_sign));

    printf("data ready to sign: %s\n", hashed_to_sign_hex);

    const char *sig = wallet_sign_string_hex_eip191(wallet->seckey, wallet->ctx, to_sign);

    printf("signature: %s\n", sig);

    long issuedAt = timestamp_now_seconds();
    long expiresAt = timestamp_seconds_from_now(3600);

    char *intent_json = sequence_build_intent_json(
        intent_data,
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

    http_response_free(&r);
    http_client_destroy(c);
    return 1;
}

sequence_wallet_t *confirm_email_sign_in(const char *email, const char *code) {
    if (!is_reasonable_email(email)) {
        fprintf(stderr, "confirm_email_sign_in: invalid email\n");
        return NULL;
    }
    if (!is_reasonable_code(code)) {
        fprintf(stderr, "confirm_email_sign_in: invalid code\n");
        return NULL;
    }

    HttpClient *c = http_client_create("https://waas.sequence.app/rpc/WaasAuthenticator");
    if (!c) {
        fprintf(stderr, "Failed to create HttpClient\n");
        return NULL;
    }

    http_add_sequence_access_key(c);
    http_client_add_header(c, "Accept: application/json");

    sequence_wallet_t *wallet = (sequence_wallet_t *)calloc(1, sizeof(sequence_wallet_t));
    if (!wallet) return NULL;

    if (!sequence_wallet_initialize(wallet)) {
        free(wallet);
        return NULL;
    }

    const char *address = sequence_wallet_get_address(wallet->ctx, &wallet->pubkey);

    const char *session_id_hex = address;

    cJSON *open_session_data = sequence_build_open_session_intent_json(
      email,
      "",
      session_id_hex,
      code
    );

    char *to_sign = cJSON_PrintUnformatted(open_session_data);

    const char *sig = wallet_sign_string_hex_eip191(wallet->seckey, wallet->ctx, to_sign);

    long issuedAt = timestamp_now_seconds();
    long expiresAt = timestamp_seconds_from_now(3600);

    char *intent_json = sequence_build_intent_json(
        open_session_data,
        "openSession",
        issuedAt,
        expiresAt,
        session_id_hex,
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

    http_response_free(&r);
    http_client_destroy(c);

    return wallet;
}

void sequence_wallet_free(sequence_wallet_t *wallet) {
    if (!wallet) return;
    sequence_wallet_clear(wallet);
    free(wallet);
}
