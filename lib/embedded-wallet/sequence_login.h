#ifndef SEQUENCE_LOGIN_H
#define SEQUENCE_LOGIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sequence_wallet.h"
#include "../networking/http_client.h"

    /*
     * Starts email sign-in:
     * typically triggers "send code to email".
     *
     * Returns:
     *   1 on success
     *   0 on failure
     */
    int sign_in_with_email(const char *email);

    /*
     * Confirms email sign-in with (email, code).
     * If successful, returns a newly allocated sequence_wallet_t*.
     *
     * Returns:
     *   non-NULL wallet pointer on success (caller must free with sequence_wallet_free)
     *   NULL on failure
     */
    sequence_wallet_t *confirm_email_sign_in(const char *email, const char *code);

    /* Frees a wallet returned by confirm_email_sign_in */
    void sequence_wallet_free(sequence_wallet_t *wallet);

#ifdef __cplusplus
}
#endif

#endif /* SEQUENCE_LOGIN_H */
