#ifndef COMMIT_VERIFIER_RETURN_H
#define COMMIT_VERIFIER_RETURN_H

typedef struct {
    char *verifier;
    char *loginHint;
    char *challenge;
} SequenceCommitVerifierResponse;

SequenceCommitVerifierResponse sequence_build_commit_verifier_return(const char *json);

void SequenceCommitVerifierResponse_free(SequenceCommitVerifierResponse *resp);

#endif // COMMIT_VERIFIER_RETURN_H