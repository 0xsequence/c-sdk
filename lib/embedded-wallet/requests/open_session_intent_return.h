typedef struct {
    char *type;
    char *sub;
    char *email;
} Identity;

typedef struct {
    char *id;
    int projectId;
    char *userId;
    Identity identity;
    char *friendlyName;
    char *createdAt;
    char *refreshedAt;
    char *expiresAt;
} Session;

typedef struct {
    char *sessionId;
    char *wallet;
} SessionResponseData;

typedef struct {
    Session session;
    char *responseCode;
    SessionResponseData responseData;
} SequenceSessionOpenedResult;

SequenceSessionOpenedResult sequence_build_open_session_intent_return(const char *json);
