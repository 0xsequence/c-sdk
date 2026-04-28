#include "wallet/oms_wallet_internal.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "evm/eoa_wallet.h"
#include "generated/waas/waas.gen.h"
#include "storage/secure_storage.h"
#include "wallet/oms_wallet_config.h"
#include "wallet/oms_wallet_request_signing.h"

#define OMS_WALLET_DEFAULT_AUTH_SIGNER_ID "default"
#define OMS_WALLET_ETHEREUM_SECP256K1_KEY_TYPE "ethereum-secp256k1"

static int default_auth_signer_create(void *ctx, char **out_signer_id)
{
    oms_wallet_sdk_t *sdk = (oms_wallet_sdk_t *)ctx;
    eoa_wallet_t wallet;
    char *signer_id;
    int status;

    if (!out_signer_id)
    {
        return EINVAL;
    }
    *out_signer_id = NULL;

    signer_id = waas_strdup(OMS_WALLET_DEFAULT_AUTH_SIGNER_ID);
    if (!signer_id)
    {
        return ENOMEM;
    }

    if (eoa_wallet_initialize(&wallet) == 0)
    {
        free(signer_id);
        return EIO;
    }

    status = secure_store_write_seckey_at(
        sdk ? sdk->config.storage_dir : NULL,
        wallet.seckey);
    eoa_wallet_destroy(&wallet);
    if (status != 0)
    {
        free(signer_id);
        return status;
    }

    *out_signer_id = signer_id;
    return 0;
}

static int default_auth_signer_delete(void *ctx, const char *signer_id)
{
    oms_wallet_sdk_t *sdk = (oms_wallet_sdk_t *)ctx;
    (void)signer_id;
    return secure_store_delete_seckey_at(sdk ? sdk->config.storage_dir : NULL);
}

static int default_auth_signer_get_credential(
    void *ctx,
    const char *signer_id,
    char **out_key_type,
    char **out_credential)
{
    oms_wallet_sdk_t *sdk = (oms_wallet_sdk_t *)ctx;
    uint8_t seckey[32];
    char *address;
    int status;

    (void)signer_id;

    if (!out_key_type || !out_credential)
    {
        return EINVAL;
    }
    *out_key_type = NULL;
    *out_credential = NULL;

    status = secure_store_read_seckey_at(sdk ? sdk->config.storage_dir : NULL, seckey);
    if (status != 0)
    {
        return status;
    }

    address = oms_wallet_address_from_seckey(seckey);
    memset(seckey, 0, sizeof(seckey));
    if (!address)
    {
        return EIO;
    }

    *out_key_type = waas_strdup(OMS_WALLET_ETHEREUM_SECP256K1_KEY_TYPE);
    if (!*out_key_type)
    {
        free(address);
        return ENOMEM;
    }

    *out_credential = address;
    return 0;
}

static int default_auth_signer_sign_authorization_message(
    void *ctx,
    const char *signer_id,
    const uint8_t *message,
    size_t message_len,
    char **out_signature_hex)
{
    oms_wallet_sdk_t *sdk = (oms_wallet_sdk_t *)ctx;
    uint8_t seckey[32];
    int status;

    (void)signer_id;

    if ((!message && message_len != 0) || !out_signature_hex)
    {
        return EINVAL;
    }
    *out_signature_hex = NULL;

    status = secure_store_read_seckey_at(sdk ? sdk->config.storage_dir : NULL, seckey);
    if (status != 0)
    {
        return status;
    }

    *out_signature_hex = oms_wallet_sign_wallet_request_preimage_bytes(
        seckey,
        message,
        message_len);
    memset(seckey, 0, sizeof(seckey));

    return *out_signature_hex ? 0 : EIO;
}

static void default_auth_signer_free_string(void *ctx, char *value)
{
    (void)ctx;
    free(value);
}

static const oms_wallet_auth_signer_provider_t default_auth_signer_provider = {
    OMS_WALLET_AUTH_SIGNER_PROVIDER_ABI_VERSION,
    NULL,
    default_auth_signer_create,
    default_auth_signer_delete,
    default_auth_signer_get_credential,
    default_auth_signer_sign_authorization_message,
    default_auth_signer_free_string
};

const oms_wallet_auth_signer_provider_t *oms_wallet_get_auth_signer_provider(oms_wallet_sdk_t *sdk)
{
    if (!sdk)
    {
        return NULL;
    }

    if (sdk->config.has_auth_signer_provider)
    {
        return &sdk->config.auth_signer_provider;
    }

    return &default_auth_signer_provider;
}

static void *oms_wallet_auth_signer_provider_ctx(
    oms_wallet_sdk_t *sdk,
    const oms_wallet_auth_signer_provider_t *provider)
{
    if (provider == &default_auth_signer_provider)
    {
        return sdk;
    }
    return provider ? provider->ctx : NULL;
}

int oms_wallet_auth_signer_create(oms_wallet_sdk_t *sdk, char **out_signer_id)
{
    const oms_wallet_auth_signer_provider_t *provider = oms_wallet_get_auth_signer_provider(sdk);
    if (!provider)
    {
        return EINVAL;
    }
    return provider->create(oms_wallet_auth_signer_provider_ctx(sdk, provider), out_signer_id);
}

int oms_wallet_auth_signer_delete(oms_wallet_sdk_t *sdk, const char *signer_id)
{
    const oms_wallet_auth_signer_provider_t *provider = oms_wallet_get_auth_signer_provider(sdk);
    if (!provider)
    {
        return EINVAL;
    }
    return provider->delete_signer(oms_wallet_auth_signer_provider_ctx(sdk, provider), signer_id);
}

int oms_wallet_auth_signer_get_credential(
    oms_wallet_sdk_t *sdk,
    const char *signer_id,
    char **out_key_type,
    char **out_credential)
{
    const oms_wallet_auth_signer_provider_t *provider = oms_wallet_get_auth_signer_provider(sdk);
    if (!provider)
    {
        return EINVAL;
    }
    return provider->get_credential(
        oms_wallet_auth_signer_provider_ctx(sdk, provider),
        signer_id,
        out_key_type,
        out_credential);
}

int oms_wallet_auth_signer_sign_authorization_message(
    oms_wallet_sdk_t *sdk,
    const char *signer_id,
    const uint8_t *message,
    size_t message_len,
    char **out_signature_hex)
{
    const oms_wallet_auth_signer_provider_t *provider = oms_wallet_get_auth_signer_provider(sdk);
    if (!provider)
    {
        return EINVAL;
    }
    return provider->sign_authorization_message(
        oms_wallet_auth_signer_provider_ctx(sdk, provider),
        signer_id,
        message,
        message_len,
        out_signature_hex);
}

void oms_wallet_auth_signer_free_string(oms_wallet_sdk_t *sdk, char *value)
{
    const oms_wallet_auth_signer_provider_t *provider = oms_wallet_get_auth_signer_provider(sdk);

    if (!value)
    {
        return;
    }

    if (provider && provider->free_string)
    {
        provider->free_string(oms_wallet_auth_signer_provider_ctx(sdk, provider), value);
    }
    else
    {
        free(value);
    }
}
