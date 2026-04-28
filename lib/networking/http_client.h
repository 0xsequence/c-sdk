/* http_client.h - C99 "class-like" HTTP POST client using libcurl */
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct HttpClient HttpClient;

    typedef struct {
        long status_code;     /* HTTP status (e.g., 200, 404) */
        char *body;           /* Response body (heap allocated, null-terminated) */
        size_t body_len;      /* Length in bytes (not counting null terminator) */
        char *error;          /* Error message if request failed (heap allocated) */
    } HttpResponse;

    /* Create/destroy */
    HttpClient* http_client_create(const char *base_url);
    void http_client_destroy(HttpClient *c);

    /* Optional: set a Bearer token header (Authorization: Bearer <token>) */
    int http_client_set_bearer_token(HttpClient *c, const char *token);

    /* Optional: set a default header (e.g. "X-API-Key: ..."). Can be called multiple times. */
    int http_client_add_header(HttpClient *c, const char *header_line);

	int http_add_oms_wallet_access_key(HttpClient *c, const char *access_key);

    /*
     * POST JSON to path (appended to base_url). Example: path="/v1/items"
     * - json_body may be NULL for empty body.
     * - timeout_ms: e.g. 10000 for 10 seconds.
     */
    HttpResponse http_client_post_json(HttpClient *c,
                                      const char *path,
                                      const char *json_body,
                                      long timeout_ms,
                                      size_t max_response_bytes);

    /* Free all heap memory inside HttpResponse (body/error) */
    void http_response_free(HttpResponse *r);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_CLIENT_H */
