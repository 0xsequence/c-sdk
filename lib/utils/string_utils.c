#include <stdlib.h>
#include <string.h>

char *concat_malloc(const char *a, const char *b) {
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);

    char *out = malloc(len_a + len_b + 1); // +1 for '\0'
    if (!out) return NULL;

    memcpy(out, a, len_a);
    memcpy(out + len_a, b, len_b);
    out[len_a + len_b] = '\0';

    return out; // caller must free()
}

char *replace_value(const char *template, const char *value)
{
    const char *placeholder = "{value}";
    const size_t ph_len = 7;

    const char *pos = strstr(template, placeholder);
    if (!pos) return NULL;

    size_t out_len =
        strlen(template)
        - ph_len
        + strlen(value)
        + 1;

    char *out = malloc(out_len);
    if (!out) return NULL;

    size_t prefix_len = pos - template;

    memcpy(out, template, prefix_len);
    memcpy(out + prefix_len, value, strlen(value));
    strcpy(out + prefix_len + strlen(value), pos + ph_len);

    return out;
}