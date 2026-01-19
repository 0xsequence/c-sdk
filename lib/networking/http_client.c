#include "http_client.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <curl/curl.h>

struct HttpClient {
    char *base_url;
    char *bearer_token;
    struct curl_slist *default_headers;
};

/* ---------- helpers ---------- */

static char* dup_cstr(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char *p = (char*)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

static char* join_url(const char *base, const char *path) {
    if (!base) return NULL;
    if (!path) path = "";

    size_t bl = strlen(base);
    size_t pl = strlen(path);

    int base_has_slash = (bl > 0 && base[bl - 1] == '/');
    int path_has_slash = (pl > 0 && path[0] == '/');

    /* Ensure exactly one slash between base and path */
    size_t extra = 1; /* for '\0' */
    size_t need = bl + pl + extra + 1; /* +1 maybe for inserting/removing slash */
    char *out = (char*)malloc(need);
    if (!out) return NULL;

    if (bl == 0) {
        snprintf(out, need, "%s", path);
        return out;
    }

    if (pl == 0) {
        snprintf(out, need, "%s", base);
        return out;
    }

    if (base_has_slash && path_has_slash) {
        /* drop one slash */
        snprintf(out, need, "%.*s%s", (int)(bl - 1), base, path);
    } else if (!base_has_slash && !path_has_slash) {
        /* add slash */
        snprintf(out, need, "%s/%s", base, path);
    } else {
        /* already exactly one slash */
        snprintf(out, need, "%s%s", base, path);
    }
    return out;
}

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} Buf;

static int buf_grow(Buf *b, size_t need) {
    if (b->cap >= need) return 1;
    size_t newcap = b->cap ? b->cap : 1024;
    while (newcap < need) newcap *= 2;
    char *p = (char*)realloc(b->data, newcap);
    if (!p) return 0;
    b->data = p;
    b->cap = newcap;
    return 1;
}

static size_t write_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t n = size * nmemb;
    Buf *b = (Buf*)userdata;

    if (!buf_grow(b, b->len + n + 1)) return 0; /* abort */
    memcpy(b->data + b->len, ptr, n);
    b->len += n;
    b->data[b->len] = '\0';
    return n;
}

static HttpResponse make_error(const char *msg) {
    HttpResponse r;
    memset(&r, 0, sizeof(r));
    r.error = dup_cstr(msg ? msg : "Unknown error");
    return r;
}

/* ---------- public API ---------- */

HttpClient* http_client_create(const char *base_url) {
    /* Safe to call many times; libcurl internally ref-counts in modern builds. */
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) return NULL;

    HttpClient *c = (HttpClient*)calloc(1, sizeof(HttpClient));
    if (!c) return NULL;

    c->base_url = dup_cstr(base_url ? base_url : "");
    if (!c->base_url) {
        free(c);
        return NULL;
    }

    return c;
}

void http_client_destroy(HttpClient *c) {
    if (!c) return;
    free(c->base_url);
    free(c->bearer_token);
    if (c->default_headers) curl_slist_free_all(c->default_headers);
    free(c);

    /* If your program creates/destroys many clients, you can move this to program shutdown. */
    curl_global_cleanup();
}

int http_client_set_bearer_token(HttpClient *c, const char *token) {
    if (!c) return 0;
    free(c->bearer_token);
    c->bearer_token = token ? dup_cstr(token) : NULL;
    return (token == NULL) || (c->bearer_token != NULL);
}

int http_client_add_header(HttpClient *c, const char *header_line) {
    if (!c || !header_line) return 0;
    struct curl_slist *newlist = curl_slist_append(c->default_headers, header_line);
    if (!newlist) return 0;
    c->default_headers = newlist;
    return 1;
}

HttpResponse http_client_post_json(HttpClient *c,
                                  const char *path,
                                  const char *json_body,
                                  long timeout_ms)
{
    if (!c) return make_error("HttpClient is NULL");

    char *url = join_url(c->base_url, path);
    if (!url) return make_error("Failed to build URL");

    CURL *curl = curl_easy_init();
    if (!curl) {
        free(url);
        return make_error("curl_easy_init failed");
    }

    Buf buf = {0};

    /* Build headers: default headers + content-type + optional auth */
    struct curl_slist *headers = NULL;

    /* Start with defaults */
    for (struct curl_slist *it = c->default_headers; it; it = it->next) {
        headers = curl_slist_append(headers, it->data);
        if (!headers) {
            curl_easy_cleanup(curl);
            free(url);
            return make_error("Failed to append default headers");
        }
    }

    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!headers) {
        curl_easy_cleanup(curl);
        free(url);
        return make_error("Failed to set Content-Type header");
    }

    if (c->bearer_token && c->bearer_token[0] != '\0') {
        size_t need = strlen("Authorization: Bearer ") + strlen(c->bearer_token) + 1;
        char *auth = (char*)malloc(need);
        if (!auth) {
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            free(url);
            return make_error("Out of memory for auth header");
        }
        snprintf(auth, need, "Authorization: Bearer %s", c->bearer_token);
        headers = curl_slist_append(headers, auth);
        free(auth);
        if (!headers) {
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            free(url);
            return make_error("Failed to append auth header");
        }
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body ? json_body : "");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)(json_body ? strlen(json_body) : 0));

    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms > 0 ? timeout_ms : 0);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    /* Response capture */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);

    /* Useful defaults */
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "c99-http-client/1.0");

    CURLcode rc = curl_easy_perform(curl);

    HttpResponse r;
    memset(&r, 0, sizeof(r));

    if (rc != CURLE_OK) {
        r.error = dup_cstr(curl_easy_strerror(rc));
    } else {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &r.status_code);
        r.body = buf.data ? buf.data : dup_cstr("");
        r.body_len = buf.len;
        buf.data = NULL; /* transfer ownership */
    }

    /* cleanup */
    free(buf.data);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(url);

    return r;
}

void http_response_free(HttpResponse *r) {
    if (!r) return;
    free(r->body);
    free(r->error);
    memset(r, 0, sizeof(*r));
}
