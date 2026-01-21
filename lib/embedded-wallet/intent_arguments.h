#ifndef INTENT_ARGUMENTS_H
#define INTENT_ARGUMENTS_H

#ifdef __cplusplus
extern "C" {
#endif

    /*
     * Builds either "initiateAuth" or "openSession" intent JSON.
     *
     * If code is NULL/empty -> initiateAuth payload:
     *   - data.metadata included
     *   - name = "initiateAuth"
     *   - NO top-level friendlyName
     *
     * If code is non-empty -> openSession payload:
     *   - data.answer = code
     *   - data.forceCreateAccount = false
     *   - data.metadata omitted
     *   - name = "openSession"
     *   - top-level friendlyName included (if friendly_name is NULL, uses "")
     *
     * Returns malloc'd JSON string (caller must free), or NULL on failure.
     */
    char *sequence_build_initiate_auth_intent_json(
        const char *email,
        const char *metadata,         /* used only when code is empty */
        const char *session_id_hex,
        long long issued_at,
        long long expires_at,
        const char *sig_session_id,
        const char *signature_hex,
        const char *version_str,
        const char *code,             /* optional; triggers openSession when non-empty */
        const char *friendly_name     /* used only when code is non-empty */
    );

#ifdef __cplusplus
}
#endif

#endif /* INTENT_ARGUMENTS_H */
