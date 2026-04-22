#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "storage/secure_storage.h"
#include "wallet/oms_wallet_config.h"

static void cleanup_storage_dir(const char *path)
{
    DIR *dir = opendir(path);
    struct dirent *entry;

    if (!dir)
    {
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        char child[PATH_MAX];

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
        unlink(child);
    }

    closedir(dir);
    rmdir(path);
}

int main(void)
{
    char template[] = "/tmp/oms-wallet-secure-storage-XXXXXX";
    char challenge_path[PATH_MAX];
    char seckey_path[PATH_MAX];
    char *storage_dir = mkdtemp(template);
    char *stored_value = NULL;
    struct stat st;
    uint8_t seckey_in[32];
    uint8_t seckey_out[32];

    if (!storage_dir)
    {
        perror("mkdtemp");
        return 1;
    }

    if (oms_wallet_config_init("test-access-key") != 0)
    {
        fprintf(stderr, "oms_wallet_config_init failed\n");
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (oms_wallet_config_set_storage_dir(storage_dir) != 0)
    {
        fprintf(stderr, "oms_wallet_config_set_storage_dir failed\n");
        oms_wallet_config_cleanup();
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (secure_store_write_string("challenge", "first-value") != 0)
    {
        fprintf(stderr, "secure_store_write_string initial write failed\n");
        oms_wallet_config_cleanup();
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (secure_store_write_string("challenge", "second-value") != 0)
    {
        fprintf(stderr, "secure_store_write_string overwrite failed\n");
        oms_wallet_config_cleanup();
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    snprintf(challenge_path, sizeof(challenge_path), "%s/challenge", storage_dir);
    snprintf(seckey_path, sizeof(seckey_path), "%s/seckey", storage_dir);

    if (secure_store_read_string("challenge", &stored_value) != 0)
    {
        fprintf(stderr, "secure_store_read_string failed\n");
        oms_wallet_config_cleanup();
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (strcmp(stored_value, "second-value") != 0)
    {
        fprintf(stderr, "unexpected stored value: %s\n", stored_value);
        free(stored_value);
        oms_wallet_config_cleanup();
        cleanup_storage_dir(storage_dir);
        return 1;
    }
    free(stored_value);

    for (size_t i = 0; i < sizeof(seckey_in); ++i)
    {
        seckey_in[i] = (uint8_t)(i + 1);
    }

    if (secure_store_write_seckey(seckey_in) != 0)
    {
        fprintf(stderr, "secure_store_write_seckey failed\n");
        oms_wallet_config_cleanup();
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (secure_store_read_seckey(seckey_out) != 0)
    {
        fprintf(stderr, "secure_store_read_seckey failed\n");
        oms_wallet_config_cleanup();
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (memcmp(seckey_in, seckey_out, sizeof(seckey_in)) != 0)
    {
        fprintf(stderr, "seckey roundtrip mismatch\n");
        oms_wallet_config_cleanup();
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (stat(storage_dir, &st) != 0)
    {
        perror("stat storage_dir");
        oms_wallet_config_cleanup();
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (!S_ISDIR(st.st_mode) || (st.st_mode & 0777) != 0700)
    {
        fprintf(stderr, "storage dir permissions mismatch: %o\n", st.st_mode & 0777);
        oms_wallet_config_cleanup();
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (stat(challenge_path, &st) != 0)
    {
        perror("stat challenge_path");
        oms_wallet_config_cleanup();
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (!S_ISREG(st.st_mode) || (st.st_mode & 0777) != 0600)
    {
        fprintf(stderr, "challenge file permissions mismatch: %o\n", st.st_mode & 0777);
        oms_wallet_config_cleanup();
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (stat(seckey_path, &st) != 0)
    {
        perror("stat seckey_path");
        oms_wallet_config_cleanup();
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (!S_ISREG(st.st_mode) || (st.st_mode & 0777) != 0600)
    {
        fprintf(stderr, "seckey file permissions mismatch: %o\n", st.st_mode & 0777);
        oms_wallet_config_cleanup();
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    oms_wallet_config_cleanup();
    cleanup_storage_dir(storage_dir);
    printf("secure_storage_test passed\n");
    return 0;
}
