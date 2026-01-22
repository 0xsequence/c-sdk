#include "sequence_login.h"
#include "intent_arguments.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --------- tiny helpers --------- */

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

/* --------- public API --------- */

int sign_in_with_email(const char *email) {
    if (!is_reasonable_email(email)) {
        fprintf(stderr, "sign_in_with_email: invalid email\n");
        return 0;
    }

    HttpClient *c = http_client_create("waas.sequence.app/rpc/WaasAuthenticator");
    if (!c) {
        fprintf(stderr, "Failed to create HttpClient\n");
        return 1;
    }

    http_add_sequence_access_key(c);
    http_client_add_header(c, "Accept: application/json");

    char *json = sequence_build_initiate_auth_intent_json(
      email,
      "",
      "0x001e...",
      1769005042LL,
      1869005072LL,
      "0x001e...",
      "0x5b08...",
      "1 (C 1.0.0)",
      NULL,
      NULL
    );

    printf("%s", json);

    HttpResponse r = http_client_post_json(c, "/SendIntent", json, 10000);

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

    HttpClient *c = http_client_create("waas.sequence.app/rpc/WaasAuthenticator");
    if (!c) {
        fprintf(stderr, "Failed to create HttpClient\n");
        return NULL;
    }

    http_add_sequence_access_key(c);
    http_client_add_header(c, "Accept: application/json");

    char *json = sequence_build_initiate_auth_intent_json(
      email,
      "",
      "0x001e...",
      1769005042LL,
      1869005072LL,
      "0x001e...",
      "0x5b08...",
      "1 (C 1.0.0)",
      code,
      NULL
    );

    HttpResponse r = http_client_post_json(c, "/RegisterSession", json, 10000);

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

    sequence_wallet_t *wallet = (sequence_wallet_t *)calloc(1, sizeof(sequence_wallet_t));
    if (!wallet) return NULL;

    if (!sequence_wallet_initialize(wallet)) {
        free(wallet);
        return NULL;
    }

    return wallet;
}

void sequence_wallet_free(sequence_wallet_t *wallet) {
    if (!wallet) return;
    sequence_wallet_clear(wallet);
    free(wallet);
}
