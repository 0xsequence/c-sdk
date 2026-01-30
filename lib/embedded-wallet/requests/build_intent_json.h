#include <cjson/cJSON.h>

char *sequence_build_intent_json(
    cJSON *data,
    const char *name,
    long long issued_at,
    long long expires_at,
    const char *sig_session_id,
    const char *signature_hex
);
