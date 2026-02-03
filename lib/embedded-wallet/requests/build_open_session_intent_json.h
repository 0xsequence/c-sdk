#include <cjson/cJSON.h>

cJSON *sequence_build_open_session_intent_json(
    const char *email,
    const char *session_id_hex,
    const char *code
);
