#include <cjson/cJSON.h>

char *build_signable_intent_json(
    cJSON *data,
    const char *name,
    long long issued_at,
    long long expires_at
);
