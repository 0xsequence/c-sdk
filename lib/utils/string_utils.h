#ifndef OMS_WALLET_STRING_UTILS_H
#define OMS_WALLET_STRING_UTILS_H

#include <stddef.h>

char *concat_malloc(const char *a, const char *b);
char *format_placeholders(const char *tmpl, const char *const *args, size_t argc);
char *replace_value(const char *template, const char *value);

#endif
