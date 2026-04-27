#include "http_client_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../wallet/oms_wallet_config.h"

char *http_dup_cstr(const char *s)
{
    size_t n;
    char *p;

    if (!s) return NULL;

    n = strlen(s);
    p = (char *)malloc(n + 1);
    if (!p) return NULL;

    memcpy(p, s, n + 1);
    return p;
}

char *http_join_url(const char *base, const char *path)
{
    size_t bl;
    size_t pl;
    int base_has_slash;
    int path_has_slash;
    size_t need;
    char *out;

    if (!base) return NULL;
    if (!path) path = "";

    bl = strlen(base);
    pl = strlen(path);
    base_has_slash = (bl > 0 && base[bl - 1] == '/');
    path_has_slash = (pl > 0 && path[0] == '/');
    need = bl + pl + 3;
    out = (char *)malloc(need);
    if (!out) return NULL;

    if (bl == 0) {
        snprintf(out, need, "%s", path);
    } else if (pl == 0) {
        snprintf(out, need, "%s", base);
    } else if (base_has_slash && path_has_slash) {
        snprintf(out, need, "%.*s%s", (int)(bl - 1), base, path);
    } else if (!base_has_slash && !path_has_slash) {
        snprintf(out, need, "%s/%s", base, path);
    } else {
        snprintf(out, need, "%s%s", base, path);
    }

    return out;
}

char *http_oms_wallet_access_key_header(void)
{
    size_t len;
    char *header;

    if (!oms_wallet_config.access_key || !oms_wallet_config.access_key[0]) {
        return NULL;
    }

    len = strlen("X-Access-Key: ") + strlen(oms_wallet_config.access_key) + 1;
    header = (char *)malloc(len);
    if (!header) return NULL;

    snprintf(header, len, "X-Access-Key: %s", oms_wallet_config.access_key);
    return header;
}
