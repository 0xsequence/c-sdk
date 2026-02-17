#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *concat_malloc(const char *a, const char *b) {
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);

    char *out = malloc(len_a + len_b + 1); // +1 for '\0'
    if (!out)
    {
        fprintf(stderr, "unable to malloc during concat_malloc");
        return NULL;
    }

    memcpy(out, a, len_a);
    memcpy(out + len_a, b, len_b);
    out[len_a + len_b] = '\0';

    return out; // caller must free()
}

static int is_placeholder(const char *p, int *index_out, size_t *tok_len_out)
{
    // Accept "{0}".."{9}" (token length always 3)
    if (p[0] == '{' && isdigit((unsigned char)p[1]) && p[2] == '}') {
        *index_out = p[1] - '0';
        *tok_len_out = 3;
        return 1;
    }
    return 0;
}

char *format_placeholders(const char *tmpl, const char *const *args, size_t argc)
{
    if (!tmpl) return NULL;

    // Pass 1: compute output length
    size_t out_len = 0;
    for (const char *p = tmpl; *p; ) {
        int idx = -1;
        size_t tok_len = 0;

        if (is_placeholder(p, &idx, &tok_len) && (size_t)idx < argc && args[idx]) {
            out_len += strlen(args[idx]);
            p += tok_len;
        } else {
            out_len += 1;
            p += 1;
        }
    }

    // +1 for null terminator
    char *out = (char *)malloc(out_len + 1);
    if (!out) {
        fprintf(stderr, "unable to malloc during format_placeholders\n");
        return NULL;
    }

    // Pass 2: build output
    char *w = out;
    for (const char *p = tmpl; *p; ) {
        int idx = -1;
        size_t tok_len = 0;

        if (is_placeholder(p, &idx, &tok_len) && (size_t)idx < argc && args[idx]) {
            size_t n = strlen(args[idx]);
            memcpy(w, args[idx], n);
            w += n;
            p += tok_len;
        } else {
            *w++ = *p++;
        }
    }
    *w = '\0';
    return out;
}

char *replace_value(const char *template, const char *value)
{
    const char *placeholder = "{value}";
    const size_t ph_len = 7;

    const char *pos = strstr(template, placeholder);
    if (!pos)
    {
        fprintf(stderr, "unable to replace value inside string");
        return NULL;
    }

    size_t out_len =
        strlen(template)
        - ph_len
        + strlen(value)
        + 1;

    char *out = malloc(out_len);
    if (!out)
    {
        fprintf(stderr, "unable to malloc during replace_value");
        return NULL;
    }

    size_t prefix_len = pos - template;

    memcpy(out, template, prefix_len);
    memcpy(out + prefix_len, value, strlen(value));
    strcpy(out + prefix_len + strlen(value), pos + ph_len);

    return out;
}