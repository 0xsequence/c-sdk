#if MBED_TLS

#pragma once
#include <stddef.h>

typedef struct HttpClient HttpClient;

typedef struct {
    long   status_code;
    char  *body;
    size_t body_len;
    char  *error;
} HttpResponse;

HttpClient* http_client_create(const char *base_url);
void http_client_destroy(HttpClient *c);

int http_client_set_bearer_token(HttpClient *c, const char *token);
int http_client_add_header(HttpClient *c, const char *header_line);
int http_add_sequence_access_key(HttpClient *c);

/* Optional but strongly recommended for HTTPS verification */
int http_client_set_ca_cert_pem(HttpClient *c, const char *ca_pem);  /* copies string */
void http_client_set_insecure_tls(HttpClient *c, int insecure);      /* 1 = skip verify */

HttpResponse http_client_post_json(HttpClient *c,
                                   const char *path,
                                   const char *json_body,
                                   long timeout_ms);

void http_response_free(HttpResponse *r);
#endif
