#ifndef COMMIT_VERIFIER_RETURN_H
#define COMMIT_VERIFIER_RETURN_H

typedef struct {
    char *verifier;
    char *loginHint;
    char *challenge;
} sequence_commit_verifier_response;

sequence_commit_verifier_response *sequence_build_commit_verifier_return(const char *json);

void sequence_commit_verifier_response_free(sequence_commit_verifier_response *resp);

#endif // COMMIT_VERIFIER_RETURN_H