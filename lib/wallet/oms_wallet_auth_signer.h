#ifndef OMS_WALLET_AUTH_SIGNER_H
#define OMS_WALLET_AUTH_SIGNER_H

#include <stddef.h>
#include <stdint.h>

#define OMS_WALLET_AUTH_SIGNER_PROVIDER_ABI_VERSION 1u

typedef struct oms_wallet_auth_signer_provider {
    uint32_t abi_version;
    void *ctx;

    /*
     * Creates or opens an auth credential and returns an opaque provider-owned
     * signer id. The SDK persists only this id; providers must not require the
     * SDK to persist or export private key bytes.
     */
    int (*create)(void *ctx, char **out_signer_id);

    /*
     * Deletes the credential identified by signer_id. Providers should make this
     * idempotent where the backing secure store allows it, because SDK cleanup
     * paths may call delete after partially completed sign-in attempts.
     */
    int (*delete_signer)(void *ctx, const char *signer_id);

    /*
     * Returns the credential identity used in the WaaS Authorization header.
     * key_type must be a WaaS-supported credential type, for example
     * "ethereum-secp256k1" or "webcrypto-secp256r1".
     */
    int (*get_credential)(
        void *ctx,
        const char *signer_id,
        char **out_key_type,
        char **out_credential);

    /*
     * Signs SDK-canonical authorization message bytes. Canonicalization is owned
     * by the SDK; providers should treat message as opaque bytes and must not
     * reconstruct the request independently.
     */
    int (*sign_authorization_message)(
        void *ctx,
        const char *signer_id,
        const uint8_t *message,
        size_t message_len,
        char **out_signature_hex);

    /*
     * Frees strings returned by this provider. If NULL, the SDK falls back to
     * free(3), so provider-returned strings must then be malloc-compatible.
     */
    void (*free_string)(void *ctx, char *value);
} oms_wallet_auth_signer_provider_t;

#endif
