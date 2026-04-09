#if MBED_TLS

#include "http_client.h"
#include "http_client_common.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "../wallet/sequence_config.h"

/* Mbed TLS */
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/error.h"

/* ================= helpers ================= */

typedef struct HeaderNode {
    char *line; /* "Key: value" */
    struct HeaderNode *next;
} HeaderNode;

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} Buf;

static int buf_grow(Buf *b, size_t need) {
    if (b->cap >= need) return 1;
    size_t newcap = b->cap ? b->cap : 2048;
    while (newcap < need) newcap *= 2;
    char *p = (char*)realloc(b->data, newcap);
    if (!p) return 0;
    b->data = p;
    b->cap = newcap;
    return 1;
}

static int buf_append(Buf *b, const void *data, size_t n) {
    if (!buf_grow(b, b->len + n + 1)) return 0;
    memcpy(b->data + b->len, data, n);
    b->len += n;
    b->data[b->len] = '\0';
    return 1;
}

static HttpResponse make_error(const char *msg) {
    HttpResponse r;
    memset(&r, 0, sizeof(r));
    r.error = http_dup_cstr(msg ? msg : "Unknown error");
    return r;
}

static HttpResponse make_mbedtls_error(const char *prefix, int err) {
    char msg[256], ebuf[160];
    mbedtls_strerror(err, ebuf, sizeof(ebuf));
    snprintf(msg, sizeof(msg), "%s: %s (0x%04x)", prefix ? prefix : "MbedTLS error", ebuf, (unsigned)(-err));
    return make_error(msg);
}

/* Very small URL parser: supports http://host[:port]/path and https://... */
typedef struct {
    int is_https;
    char host[256];
    char port[8];      /* "80"/"443"/custom */
    char path[1024];   /* starts with '/' */
} ParsedUrl;

static int parse_url(const char *url, ParsedUrl *out) {
    if (!url || !out) return 0;
    memset(out, 0, sizeof(*out));

    const char *p = NULL;
    if (strncmp(url, "https://", 8) == 0) {
        out->is_https = 1;
        p = url + 8;
        strcpy(out->port, "443");
    } else if (strncmp(url, "http://", 7) == 0) {
        out->is_https = 0;
        p = url + 7;
        strcpy(out->port, "80");
    } else {
        /* If no scheme, assume https is NOT safe; pick http by default */
        out->is_https = 0;
        p = url;
        strcpy(out->port, "80");
    }

    /* host[:port][/path] */
    const char *host_start = p;
    const char *slash = strchr(p, '/');
    const char *host_end = slash ? slash : (p + strlen(p));

    /* optional :port in host part */
    const char *colon = NULL;
    for (const char *q = host_start; q < host_end; ++q) {
        if (*q == ':') { colon = q; break; }
    }

    size_t host_len = colon ? (size_t)(colon - host_start) : (size_t)(host_end - host_start);
    if (host_len == 0 || host_len >= sizeof(out->host)) return 0;
    memcpy(out->host, host_start, host_len);
    out->host[host_len] = '\0';

    if (colon) {
        size_t port_len = (size_t)(host_end - (colon + 1));
        if (port_len == 0 || port_len >= sizeof(out->port)) return 0;
        memcpy(out->port, colon + 1, port_len);
        out->port[port_len] = '\0';
    }

    if (slash) {
        size_t path_len = strlen(slash);
        if (path_len >= sizeof(out->path)) return 0;
        memcpy(out->path, slash, path_len + 1);
    } else {
        strcpy(out->path, "/");
    }

    return 1;
}

/* Find end of headers (\r\n\r\n) */
static int find_header_end(const char *s, size_t n, size_t *out_pos) {
    if (!s || n < 4) return 0;
    for (size_t i = 0; i + 3 < n; ++i) {
        if (s[i] == '\r' && s[i+1] == '\n' && s[i+2] == '\r' && s[i+3] == '\n') {
            *out_pos = i + 4;
            return 1;
        }
    }
    return 0;
}

/* Case-insensitive header lookup in a raw header block */
static int header_get_value(const char *headers, const char *key, char *out, size_t out_sz) {
    /* headers: lines ending with \r\n, last line blank */
    if (!headers || !key || !out || out_sz == 0) return 0;

    size_t key_len = strlen(key);
    const char *p = headers;
    while (*p) {
        const char *line_end = strstr(p, "\r\n");
        if (!line_end) break;
        if (line_end == p) break; /* blank line */
        const char *colon = memchr(p, ':', (size_t)(line_end - p));
        if (colon) {
            size_t name_len = (size_t)(colon - p);
            if (name_len == key_len) {
                int match = 1;
                for (size_t i = 0; i < key_len; ++i) {
                    char a = (char)tolower((unsigned char)p[i]);
                    char b = (char)tolower((unsigned char)key[i]);
                    if (a != b) { match = 0; break; }
                }
                if (match) {
                    const char *v = colon + 1;
                    while (*v == ' ' || *v == '\t') v++;
                    size_t vlen = (size_t)(line_end - v);
                    if (vlen >= out_sz) vlen = out_sz - 1;
                    memcpy(out, v, vlen);
                    out[vlen] = '\0';
                    return 1;
                }
            }
        }
        p = line_end + 2;
    }
    return 0;
}

/* Parse status code from "HTTP/1.1 200 OK" */
static long parse_status_code(const char *headers) {
    if (!headers) return 0;
    const char *sp = strchr(headers, ' ');
    if (!sp) return 0;
    while (*sp == ' ') sp++;
    return strtol(sp, NULL, 10);
}

/* Decode chunked transfer encoding body (expects chunked data only) */
static int decode_chunked(const char *in, size_t in_len, Buf *out) {
    size_t i = 0;
    while (i < in_len) {
        /* read hex size line until \r\n */
        size_t line_start = i;
        const char *eol = NULL;
        for (size_t j = i; j + 1 < in_len; ++j) {
            if (in[j] == '\r' && in[j+1] == '\n') { eol = in + j; break; }
        }
        if (!eol) return 0;
        size_t line_len = (size_t)(eol - (in + line_start));
        if (line_len == 0) return 0;

        /* chunk extensions ignored: parse until ';' */
        size_t parse_len = line_len;
        for (size_t k = 0; k < line_len; ++k) {
            if (in[line_start + k] == ';') { parse_len = k; break; }
        }

        char hex[32];
        if (parse_len >= sizeof(hex)) return 0;
        memcpy(hex, in + line_start, parse_len);
        hex[parse_len] = '\0';

        size_t chunk_sz = (size_t)strtoul(hex, NULL, 16);

        i = (size_t)((eol - in) + 2); /* after \r\n */
        if (chunk_sz == 0) {
            /* consume trailing headers if present; we can stop */
            return 1;
        }

        if (i + chunk_sz + 2 > in_len) return 0; /* need chunk data + \r\n */
        if (!buf_append(out, in + i, chunk_sz)) return 0;
        i += chunk_sz;

        /* must end with \r\n */
        if (in[i] != '\r' || in[i+1] != '\n') return 0;
        i += 2;
    }
    return 1;
}

/* ================= client ================= */

struct HttpClient {
    char *base_url;
    char *bearer_token;
    HeaderNode *default_headers;

    char *ca_cert_pem;     /* optional PEM bundle */
    int insecure_tls;      /* 1 = skip verify (not recommended) */
};

HttpClient* http_client_create(const char *base_url) {
    HttpClient *c = (HttpClient*)calloc(1, sizeof(HttpClient));
    if (!c) return NULL;
    c->base_url = http_dup_cstr(base_url ? base_url : "");
    if (!c->base_url) { free(c); return NULL; }
    c->insecure_tls = 0;
    return c;
}

void http_client_destroy(HttpClient *c) {
    if (!c) return;
    free(c->base_url);
    free(c->bearer_token);
    free(c->ca_cert_pem);
    HeaderNode *h = c->default_headers;
    while (h) { HeaderNode *n = h->next; free(h->line); free(h); h = n; }
    free(c);
}

int http_client_set_bearer_token(HttpClient *c, const char *token) {
    if (!c) return 0;
    free(c->bearer_token);
    c->bearer_token = token ? http_dup_cstr(token) : NULL;
    return (token == NULL) || (c->bearer_token != NULL);
}

int http_client_add_header(HttpClient *c, const char *header_line) {
    if (!c || !header_line) return 0;
    HeaderNode *n = (HeaderNode*)calloc(1, sizeof(HeaderNode));
    if (!n) return 0;
    n->line = http_dup_cstr(header_line);
    if (!n->line) { free(n); return 0; }
    n->next = NULL;

    if (!c->default_headers) {
        c->default_headers = n;
    } else {
        HeaderNode *it = c->default_headers;
        while (it->next) it = it->next;
        it->next = n;
    }
    return 1;
}

int http_add_sequence_access_key(HttpClient *c) {
    char *header;

    if (!c || !sequence_config.access_key || !sequence_config.access_key[0]) return 0;

    header = http_sequence_access_key_header();
    if (!header) return -1;
    int ok = http_client_add_header(c, header);
    free(header);
    return ok;
}

int http_client_set_ca_cert_pem(HttpClient *c, const char *ca_pem) {
    if (!c) return 0;
    free(c->ca_cert_pem);
    c->ca_cert_pem = ca_pem ? http_dup_cstr(ca_pem) : NULL;
    return (ca_pem == NULL) || (c->ca_cert_pem != NULL);
}

void http_client_set_insecure_tls(HttpClient *c, int insecure) {
    if (!c) return;
    c->insecure_tls = insecure ? 1 : 0;
}

/* Build request into Buf */
static int build_post_request(Buf *req,
                              const ParsedUrl *pu,
                              const HttpClient *c,
                              const char *json_body)
{
    const char *body = json_body ? json_body : "";
    size_t body_len = strlen(body);

    /* Request line + required headers */
    char head[2048];
    int n = snprintf(head, sizeof(head),
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: c99-mbedtls-http/1.0\r\n"
        "Accept: */*\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n",
        pu->path, pu->host, body_len);
    if (n <= 0 || (size_t)n >= sizeof(head)) return 0;
    if (!buf_append(req, head, (size_t)n)) return 0;

    /* Default headers */
    for (HeaderNode *it = c->default_headers; it; it = it->next) {
        if (!buf_append(req, it->line, strlen(it->line))) return 0;
        if (!buf_append(req, "\r\n", 2)) return 0;
    }

    /* Authorization: Bearer */
    if (c->bearer_token && c->bearer_token[0]) {
        char auth[1024];
        int an = snprintf(auth, sizeof(auth), "Authorization: Bearer %s\r\n", c->bearer_token);
        if (an <= 0 || (size_t)an >= sizeof(auth)) return 0;
        if (!buf_append(req, auth, (size_t)an)) return 0;
    }

    /* End headers */
    if (!buf_append(req, "\r\n", 2)) return 0;

    /* Body */
    if (body_len) {
        if (!buf_append(req, body, body_len)) return 0;
    }
    return 1;
}

/* Read all bytes from TLS or plain socket into Buf until close or timeout.
   We rely on mbedtls_net_set_timeout for socket-level timeouts.
*/
static int read_all_tls(mbedtls_ssl_context *ssl, Buf *out) {
    unsigned char tmp[2048];
    for (;;) {
        int r = mbedtls_ssl_read(ssl, tmp, sizeof(tmp));
        if (r == 0) return 1; /* close notify */
        if (r == MBEDTLS_ERR_SSL_WANT_READ || r == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
        if (r < 0) return 0;
        if (!buf_append(out, tmp, (size_t)r)) return 0;
    }
}

static int read_all_plain(mbedtls_net_context *net, Buf *out) {
    unsigned char tmp[2048];
    for (;;) {
        int r = mbedtls_net_recv(net, tmp, sizeof(tmp));
        if (r == 0) return 1;
        if (r == MBEDTLS_ERR_SSL_WANT_READ || r == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
        if (r < 0) return 0;
        if (!buf_append(out, tmp, (size_t)r)) return 0;
    }
}

/* ================= main POST ================= */

HttpResponse http_client_post_json(HttpClient *c,
                                  const char *path,
                                  const char *json_body,
                                  long timeout_ms)
{
    if (!c) return make_error("HttpClient is NULL");

    char *url = http_join_url(c->base_url, path);
    if (!url) return make_error("Failed to build URL");

    ParsedUrl pu;
    if (!parse_url(url, &pu)) {
        free(url);
        return make_error("Failed to parse URL");
    }

    /* --- Build HTTP request bytes --- */
    Buf req = {0};
    if (!build_post_request(&req, &pu, c, json_body)) {
        free(req.data);
        free(url);
        return make_error("Failed to build HTTP request");
    }

    /* --- Networking/TLS setup --- */
    int ret = 0;
    HttpResponse resp;
    memset(&resp, 0, sizeof(resp));

    mbedtls_net_context net;
    mbedtls_net_init(&net);

    ret = mbedtls_net_connect(&net, pu.host, pu.port, MBEDTLS_NET_PROTO_TCP);
    if (ret != 0) {
        free(req.data);
        free(url);
        mbedtls_net_free(&net);
        return make_mbedtls_error("TCP connect failed", ret);
    }

    Buf raw = {0};

    if (!pu.is_https) {
        /* ---- Plain HTTP ---- */
        size_t written = 0;
        while (written < req.len) {
            ret = mbedtls_net_send(&net, (const unsigned char*)req.data + written,
                                   (size_t)(req.len - written));
            if (ret <= 0) { resp = make_error("HTTP send failed"); goto done; }
            written += (size_t)ret;
        }

        if (!read_all_plain(&net, &raw)) {
            resp = make_error("HTTP read failed");
            goto done;
        }
    } else {
        /* ---- HTTPS via Mbed TLS ---- */
        mbedtls_ssl_context ssl;
        mbedtls_ssl_config conf;
        mbedtls_entropy_context entropy;
        mbedtls_ctr_drbg_context ctr_drbg;
        mbedtls_x509_crt cacert;

        mbedtls_ssl_init(&ssl);
        mbedtls_ssl_config_init(&conf);
        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&ctr_drbg);
        mbedtls_x509_crt_init(&cacert);

        const char *pers = "mbedtls_http_post";
        ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                    (const unsigned char*)pers, strlen(pers));
        if (ret != 0) { resp = make_mbedtls_error("ctr_drbg_seed failed", ret); goto tls_done; }

        ret = mbedtls_ssl_config_defaults(&conf,
                                          MBEDTLS_SSL_IS_CLIENT,
                                          MBEDTLS_SSL_TRANSPORT_STREAM,
                                          MBEDTLS_SSL_PRESET_DEFAULT);

		if (timeout_ms > 0) {
    		mbedtls_ssl_conf_read_timeout(&conf, (uint32_t)timeout_ms);
		}

        if (ret != 0) { resp = make_mbedtls_error("ssl_config_defaults failed", ret); goto tls_done; }


        mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

        /* Certificate verification */
        if (c->insecure_tls) {
            mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
        } else if (c->ca_cert_pem && c->ca_cert_pem[0]) {
            ret = mbedtls_x509_crt_parse(&cacert,
                                         (const unsigned char*)c->ca_cert_pem,
                                         strlen(c->ca_cert_pem) + 1);
            if (ret < 0) { resp = make_mbedtls_error("CA cert parse failed", ret); goto tls_done; }
            mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
            mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
        } else {
            /* Safer default is to FAIL here, but many embedded projects want an escape hatch.
               Choose behavior: either fail or allow insecure. */
            resp = make_error("No CA cert configured for HTTPS. Provide CA PEM or enable insecure TLS.");
            goto tls_done;
        }

        ret = mbedtls_ssl_setup(&ssl, &conf);
        if (ret != 0) { resp = make_mbedtls_error("ssl_setup failed", ret); goto tls_done; }

        /* SNI / hostname verification */
        ret = mbedtls_ssl_set_hostname(&ssl, pu.host);
        if (ret != 0) { resp = make_mbedtls_error("set_hostname failed", ret); goto tls_done; }

        mbedtls_ssl_set_bio(&ssl, &net, mbedtls_net_send, mbedtls_net_recv, NULL);

        while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
            if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
            resp = make_mbedtls_error("TLS handshake failed", ret);
            goto tls_done;
        }

        /* Verify result (only meaningful if VERIFY_REQUIRED) */
        if (!c->insecure_tls) {
            uint32_t flags = mbedtls_ssl_get_verify_result(&ssl);
            if (flags != 0) {
                resp = make_error("TLS certificate verification failed");
                goto tls_done;
            }
        }

        /* write request */
        size_t written = 0;
        while (written < req.len) {
            ret = mbedtls_ssl_write(&ssl, (const unsigned char*)req.data + written,
                                    (size_t)(req.len - written));
            if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
            if (ret < 0) { resp = make_mbedtls_error("TLS write failed", ret); goto tls_done; }
            written += (size_t)ret;
        }

        /* read response */
        if (!read_all_tls(&ssl, &raw)) {
            resp = make_error("TLS read failed");
            goto tls_done;
        }

        /* close */
        (void)mbedtls_ssl_close_notify(&ssl);

tls_done:
        mbedtls_x509_crt_free(&cacert);
        mbedtls_ssl_free(&ssl);
        mbedtls_ssl_config_free(&conf);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);

        if (resp.error) goto done;
    }

    /* --- Parse raw HTTP response --- */
    size_t header_end = 0;
    if (!find_header_end(raw.data, raw.len, &header_end)) {
        resp = make_error("Invalid HTTP response (no header terminator)");
        goto done;
    }

    /* Copy headers block (null-terminated) */
    Buf headers = {0};
    if (!buf_append(&headers, raw.data, header_end)) {
        free(headers.data);
        resp = make_error("Out of memory parsing headers");
        goto done;
    }

    resp.status_code = parse_status_code(headers.data);

    const char *body_ptr = raw.data + header_end;
    size_t body_len = raw.len - header_end;

    char te[64];
    char cl[64];
    int has_te = header_get_value(headers.data, "Transfer-Encoding", te, sizeof(te));
    if (has_te) {
        for (char *p = te; *p; ++p) *p = (char)tolower((unsigned char)*p);
    }

    if (has_te && strstr(te, "chunked") != NULL) {
        Buf decoded = {0};
        if (!decode_chunked(body_ptr, body_len, &decoded)) {
            free(decoded.data);
            free(headers.data);
            resp = make_error("Failed to decode chunked body");
            goto done;
        }
        resp.body = decoded.data ? decoded.data : http_dup_cstr("");
        resp.body_len = decoded.len;
    } else if (header_get_value(headers.data, "Content-Length", cl, sizeof(cl))) {
        size_t want = (size_t)strtoul(cl, NULL, 10);
        if (want > body_len) want = body_len; /* tolerate short read */
        resp.body = (char*)malloc(want + 1);
        if (!resp.body) {
            free(headers.data);
            resp = make_error("Out of memory allocating body");
            goto done;
        }
        memcpy(resp.body, body_ptr, want);
        resp.body[want] = '\0';
        resp.body_len = want;
    } else {
        /* read-until-close already done; take whatever remains */
        resp.body = (char*)malloc(body_len + 1);
        if (!resp.body) {
            free(headers.data);
            resp = make_error("Out of memory allocating body");
            goto done;
        }
        memcpy(resp.body, body_ptr, body_len);
        resp.body[body_len] = '\0';
        resp.body_len = body_len;
    }

    free(headers.data);

done:
    free(req.data);
    free(raw.data);
    free(url);
    mbedtls_net_free(&net);
    return resp;
}

void http_response_free(HttpResponse *r) {
    if (!r) return;
    free(r->body);
    free(r->error);
    memset(r, 0, sizeof(*r));
}

#endif
