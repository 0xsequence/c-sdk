#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage/secure_storage.h"
#include "wallet/oms_wallet.h"
#include "wallet/oms_wallet_config.h"
#include "wallet/oms_wallet_internal.h"

typedef struct {
    int create_count;
    int delete_count;
    int credential_count;
    int sign_count;
    char last_message[256];
} fake_provider_ctx;

static char *dup_string(const char *value)
{
    size_t len;
    char *copy;

    if (!value)
    {
        return NULL;
    }

    len = strlen(value) + 1;
    copy = malloc(len);
    if (!copy)
    {
        return NULL;
    }
    memcpy(copy, value, len);
    return copy;
}

static int fake_create(void *ctx, char **out_signer_id)
{
    fake_provider_ctx *fake = ctx;

    if (!fake || !out_signer_id)
    {
        return EINVAL;
    }

    fake->create_count++;
    *out_signer_id = dup_string("fake-signer");
    return *out_signer_id ? 0 : ENOMEM;
}

static int fake_delete(void *ctx, const char *signer_id)
{
    fake_provider_ctx *fake = ctx;

    if (!fake || !signer_id)
    {
        return EINVAL;
    }

    fake->delete_count++;
    return strcmp(signer_id, "fake-signer") == 0 ? 0 : ENOENT;
}

static int fake_get_credential(
    void *ctx,
    const char *signer_id,
    char **out_key_type,
    char **out_credential)
{
    fake_provider_ctx *fake = ctx;

    if (!fake || !signer_id || !out_key_type || !out_credential)
    {
        return EINVAL;
    }

    fake->credential_count++;
    *out_key_type = dup_string("ethereum-secp256k1");
    *out_credential = dup_string("0xfakecredential");
    return *out_key_type && *out_credential ? 0 : ENOMEM;
}

static int fake_sign_authorization_message(
    void *ctx,
    const char *signer_id,
    const uint8_t *message,
    size_t message_len,
    char **out_signature_hex)
{
    fake_provider_ctx *fake = ctx;

    if (!fake || !signer_id || (!message && message_len != 0) || !out_signature_hex)
    {
        return EINVAL;
    }

    fake->sign_count++;
    snprintf(fake->last_message, sizeof(fake->last_message), "%.*s", (int)message_len, message);

    *out_signature_hex = dup_string("0xfakesignature");
    return *out_signature_hex ? 0 : ENOMEM;
}

static void fake_free_string(void *ctx, char *value)
{
    (void)ctx;
    free(value);
}

static void expect_int(const char *label, int actual, int expected)
{
    if (actual != expected)
    {
        fprintf(stderr, "%s mismatch: expected %d, got %d\n", label, expected, actual);
        exit(1);
    }
}

static void expect_string(const char *label, const char *actual, const char *expected)
{
    if (!actual || strcmp(actual, expected) != 0)
    {
        fprintf(stderr, "%s mismatch: expected %s, got %s\n",
            label,
            expected ? expected : "(null)",
            actual ? actual : "(null)");
        exit(1);
    }
}

int main(void)
{
#ifndef __APPLE__
    char template[] = "/tmp/oms-wallet-auth-signer-provider-XXXXXX";
    char *storage_dir = mkdtemp(template);
    char stored_signer_path[PATH_MAX];
#endif
    fake_provider_ctx fake = {0};
    oms_wallet_auth_signer_provider_t provider = {
        OMS_WALLET_AUTH_SIGNER_PROVIDER_ABI_VERSION,
        &fake,
        fake_create,
        fake_delete,
        fake_get_credential,
        fake_sign_authorization_message,
        fake_free_string
    };
    char *signer_id = NULL;
    char *key_type = NULL;
    char *credential = NULL;
    char *signature = NULL;
    oms_wallet_sdk_t sdk;
#ifndef __APPLE__
    int restore_status;
#endif

#ifndef __APPLE__
    if (!storage_dir)
    {
        perror("mkdtemp");
        return 1;
    }
#endif

    if (oms_wallet_sdk_init(&sdk, "test-access-key") != 0 ||
#ifndef __APPLE__
        oms_wallet_config_set_storage_dir(&sdk, storage_dir) != 0 ||
#endif
        oms_wallet_config_set_auth_signer_provider(&sdk, &provider) != 0)
    {
        fprintf(stderr, "failed to initialize test config\n");
        return 1;
    }

    if (oms_wallet_auth_signer_create(&sdk, &signer_id) != 0)
    {
        fprintf(stderr, "create failed\n");
        return 1;
    }
    expect_string("signer id", signer_id, "fake-signer");
    expect_int("create count", fake.create_count, 1);

    if (oms_wallet_auth_signer_get_credential(&sdk, signer_id, &key_type, &credential) != 0)
    {
        fprintf(stderr, "get credential failed\n");
        return 1;
    }
    expect_string("key type", key_type, "ethereum-secp256k1");
    expect_string("credential", credential, "0xfakecredential");

    if (oms_wallet_auth_signer_sign_authorization_message(
            &sdk,
            signer_id,
            (const uint8_t *)"POST /rpc/Wallet/CreateWallet\nnonce: 42\n\n{\"type\":\"ethereum\"}",
            strlen("POST /rpc/Wallet/CreateWallet\nnonce: 42\n\n{\"type\":\"ethereum\"}"),
            &signature) != 0)
    {
        fprintf(stderr, "sign failed\n");
        return 1;
    }
    expect_string("signature", signature, "0xfakesignature");
    expect_string(
        "captured message",
        fake.last_message,
        "POST /rpc/Wallet/CreateWallet\nnonce: 42\n\n{\"type\":\"ethereum\"}");

#ifndef __APPLE__
    if (oms_wallet_session_write_string(&sdk, "oms_auth_signer_id", signer_id) != 0)
    {
        fprintf(stderr, "failed to persist signer id\n");
        return 1;
    }

    restore_status = oms_wallet_restore_session(&sdk);
    expect_int("restore status without workflow state", restore_status, 0);
    expect_int("credential count after restore", fake.credential_count, 2);
#endif

    oms_wallet_auth_signer_free_string(&sdk, signature);
    oms_wallet_auth_signer_free_string(&sdk, credential);
    oms_wallet_auth_signer_free_string(&sdk, key_type);
    oms_wallet_auth_signer_free_string(&sdk, signer_id);
    oms_wallet_sdk_cleanup(&sdk);
#ifndef __APPLE__
    snprintf(stored_signer_path, sizeof(stored_signer_path), "%s/oms_auth_signer_id", storage_dir);
    unlink(stored_signer_path);
    rmdir(storage_dir);
#endif

    printf("auth_signer_provider_test passed\n");
    return 0;
}
