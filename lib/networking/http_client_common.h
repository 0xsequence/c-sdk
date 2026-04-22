#ifndef OMS_WALLET_HTTP_CLIENT_COMMON_H
#define OMS_WALLET_HTTP_CLIENT_COMMON_H

char *http_dup_cstr(const char *s);
char *http_join_url(const char *base, const char *path);
char *http_oms_wallet_access_key_header(void);

#endif
