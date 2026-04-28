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
    char *missing_value = NULL;
    struct stat st;
    uint8_t seckey_in[32];
    uint8_t seckey_out[32];
    int status;

    if (!storage_dir)
    {
        perror("mkdtemp");
        return 1;
    }

    if (secure_store_write_string_at(storage_dir, "challenge", "first-value") != 0)
    {
        fprintf(stderr, "secure_store_write_string initial write failed\n");
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (secure_store_write_string_at(storage_dir, "challenge", "second-value") != 0)
    {
        fprintf(stderr, "secure_store_write_string overwrite failed\n");
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    snprintf(challenge_path, sizeof(challenge_path), "%s/challenge", storage_dir);
    snprintf(seckey_path, sizeof(seckey_path), "%s/seckey", storage_dir);

    if (secure_store_read_string_at(storage_dir, "challenge", &stored_value) != 0)
    {
        fprintf(stderr, "secure_store_read_string failed\n");
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (strcmp(stored_value, "second-value") != 0)
    {
        fprintf(stderr, "unexpected stored value: %s\n", stored_value);
        free(stored_value);
        cleanup_storage_dir(storage_dir);
        return 1;
    }
    free(stored_value);

    status = secure_store_read_string_at(storage_dir, "missing", &missing_value);
    if (!secure_store_status_is_not_found(status))
    {
        fprintf(stderr, "missing-key status mismatch: %d\n", status);
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (secure_store_delete_at(storage_dir, "challenge") != 0)
    {
        fprintf(stderr, "secure_store_delete existing key failed\n");
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    status = secure_store_read_string_at(storage_dir, "challenge", &missing_value);
    if (!secure_store_status_is_not_found(status))
    {
        fprintf(stderr, "deleted-key status mismatch: %d\n", status);
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (secure_store_delete_at(storage_dir, "challenge") != 0)
    {
        fprintf(stderr, "secure_store_delete missing key failed\n");
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (secure_store_write_string_at(storage_dir, "challenge", "second-value") != 0)
    {
        fprintf(stderr, "secure_store_write_string rewrite failed\n");
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    for (size_t i = 0; i < sizeof(seckey_in); ++i)
    {
        seckey_in[i] = (uint8_t)(i + 1);
    }

    if (secure_store_write_seckey_at(storage_dir, seckey_in) != 0)
    {
        fprintf(stderr, "secure_store_write_seckey failed\n");
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (secure_store_read_seckey_at(storage_dir, seckey_out) != 0)
    {
        fprintf(stderr, "secure_store_read_seckey failed\n");
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (memcmp(seckey_in, seckey_out, sizeof(seckey_in)) != 0)
    {
        fprintf(stderr, "seckey roundtrip mismatch\n");
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (stat(storage_dir, &st) != 0)
    {
        perror("stat storage_dir");
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (!S_ISDIR(st.st_mode) || (st.st_mode & 0777) != 0700)
    {
        fprintf(stderr, "storage dir permissions mismatch: %o\n", st.st_mode & 0777);
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (stat(challenge_path, &st) != 0)
    {
        perror("stat challenge_path");
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (!S_ISREG(st.st_mode) || (st.st_mode & 0777) != 0600)
    {
        fprintf(stderr, "challenge file permissions mismatch: %o\n", st.st_mode & 0777);
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (stat(seckey_path, &st) != 0)
    {
        perror("stat seckey_path");
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    if (!S_ISREG(st.st_mode) || (st.st_mode & 0777) != 0600)
    {
        fprintf(stderr, "seckey file permissions mismatch: %o\n", st.st_mode & 0777);
        cleanup_storage_dir(storage_dir);
        return 1;
    }

    cleanup_storage_dir(storage_dir);
    printf("secure_storage_test passed\n");
    return 0;
}
