#include <stdlib.h>

typedef struct {
    char *sessionId;
    char *identityType;
    int expiresIn;
    char *challenge;
} AuthData;

typedef struct {
    char *code;
    AuthData data;
} SequenceInitiateAuthResponse;

SequenceInitiateAuthResponse sequence_build_initiate_auth_intent_return(const char *json);