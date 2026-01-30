#include <cjson/cJSON.h>

cJSON *sequence_build_initiate_auth_intent_json(
    const char *email,
    const char *metadata,
    const char *session_id_hex
);
