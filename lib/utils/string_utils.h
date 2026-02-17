char *concat_malloc(const char *a, const char *b);
char *format_placeholders(const char *tmpl, const char *const *args, size_t argc);
char *replace_value(const char *template, const char *value);