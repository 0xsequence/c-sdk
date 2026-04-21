#include "secure_storage.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "wallet/sequence_config.h"

#define SEQUENCE_DEFAULT_STORAGE_DIR ".sequence-c-sdk"

static char sanitize_key_char(char c)
{
    if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.')
    {
        return c;
    }

    return '_';
}

static char *sequence_storage_dir_dup(void)
{
    const char *home;
    const char *dir = sequence_config.storage_dir;

    if (dir && dir[0] != '\0')
    {
        size_t len = strlen(dir) + 1;
        char *copy = malloc(len);

        if (!copy)
        {
            return NULL;
        }
        memcpy(copy, dir, len);
        return copy;
    }

    home = getenv("HOME");
    if (home && home[0] != '\0')
    {
        size_t len = strlen(home) + 1 + strlen(SEQUENCE_DEFAULT_STORAGE_DIR) + 1;
        char *path = malloc(len);

        if (!path)
        {
            return NULL;
        }
        snprintf(path, len, "%s/%s", home, SEQUENCE_DEFAULT_STORAGE_DIR);
        return path;
    }

    {
        size_t len = strlen(SEQUENCE_DEFAULT_STORAGE_DIR) + 1;
        char *path = malloc(len);

        if (!path)
        {
            return NULL;
        }
        memcpy(path, SEQUENCE_DEFAULT_STORAGE_DIR, len);
        return path;
    }
}

static int ensure_storage_dir(const char *dir)
{
    struct stat st;

    if (!dir || dir[0] == '\0')
    {
        return EINVAL;
    }

    if (mkdir(dir, 0700) == 0)
    {
        return 0;
    }

    if (errno != EEXIST)
    {
        return errno;
    }

    if (stat(dir, &st) != 0)
    {
        return errno;
    }

    if (!S_ISDIR(st.st_mode))
    {
        return ENOTDIR;
    }

    if (chmod(dir, 0700) != 0)
    {
        return errno;
    }

    return 0;
}

static char *build_storage_path(const char *key)
{
    char *dir;
    char *path;
    size_t dir_len;
    size_t key_len;
    size_t path_len;

    if (!key || key[0] == '\0')
    {
        errno = EINVAL;
        return NULL;
    }

    dir = sequence_storage_dir_dup();
    if (!dir)
    {
        errno = ENOMEM;
        return NULL;
    }

    dir_len = strlen(dir);
    key_len = strlen(key);
    path_len = dir_len + 1 + key_len + 1;
    path = malloc(path_len);
    if (!path)
    {
        free(dir);
        errno = ENOMEM;
        return NULL;
    }

    snprintf(path, path_len, "%s/", dir);
    for (size_t i = 0; i < key_len; ++i)
    {
        path[dir_len + 1 + i] = sanitize_key_char(key[i]);
    }
    path[path_len - 1] = '\0';

    free(dir);
    return path;
}

static int write_file_atomic(const char *path, const void *data, size_t len)
{
    char *tmp_path;
    int fd;
    int status = 0;
    size_t path_len;
    ssize_t written;
    size_t total = 0;

    if (!path || (!data && len != 0))
    {
        return EINVAL;
    }

    path_len = strlen(path);
    tmp_path = malloc(path_len + strlen(".tmpXXXXXX") + 1);
    if (!tmp_path)
    {
        return ENOMEM;
    }

    snprintf(tmp_path, path_len + strlen(".tmpXXXXXX") + 1, "%s.tmpXXXXXX", path);
    fd = mkstemp(tmp_path);
    if (fd < 0)
    {
        status = errno;
        free(tmp_path);
        return status;
    }

    if (fchmod(fd, 0600) != 0)
    {
        status = errno;
        goto cleanup;
    }

    while (total < len)
    {
        written = write(fd, (const uint8_t *)data + total, len - total);
        if (written < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            status = errno;
            goto cleanup;
        }
        total += (size_t)written;
    }

    if (fsync(fd) != 0)
    {
        status = errno;
        goto cleanup;
    }

    if (close(fd) != 0)
    {
        fd = -1;
        status = errno;
        goto cleanup;
    }
    fd = -1;

    if (rename(tmp_path, path) != 0)
    {
        status = errno;
        goto cleanup;
    }

cleanup:
    if (fd >= 0)
    {
        close(fd);
    }
    if (status != 0)
    {
        unlink(tmp_path);
    }
    free(tmp_path);
    return status;
}

static int read_file_all(const char *path, uint8_t **out_data, size_t *out_len)
{
    struct stat st;
    int fd;
    uint8_t *data;
    size_t total = 0;

    if (!path || !out_data || !out_len)
    {
        return EINVAL;
    }

    *out_data = NULL;
    *out_len = 0;

    if (stat(path, &st) != 0)
    {
        return errno;
    }

    if (st.st_size < 0)
    {
        return EINVAL;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        return errno;
    }

    data = malloc((size_t)st.st_size);
    if (!data && st.st_size != 0)
    {
        close(fd);
        return ENOMEM;
    }

    while (total < (size_t)st.st_size)
    {
        ssize_t nread = read(fd, data + total, (size_t)st.st_size - total);

        if (nread < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            free(data);
            close(fd);
            return errno;
        }
        if (nread == 0)
        {
            break;
        }

        total += (size_t)nread;
    }

    close(fd);
    *out_data = data;
    *out_len = total;
    return 0;
}

int secure_store_write_string(const char *key, const char *value)
{
    char *dir;
    char *path;
    int status;

    if (!key || !value)
    {
        return EINVAL;
    }

    dir = sequence_storage_dir_dup();
    if (!dir)
    {
        return ENOMEM;
    }

    status = ensure_storage_dir(dir);
    free(dir);
    if (status != 0)
    {
        return status;
    }

    path = build_storage_path(key);
    if (!path)
    {
        return errno ? errno : ENOMEM;
    }

    status = write_file_atomic(path, value, strlen(value));
    free(path);
    return status;
}

int secure_store_read_string(const char *key, char **value)
{
    char *path;
    uint8_t *data = NULL;
    size_t len = 0;
    int status;

    if (!key || !value)
    {
        return EINVAL;
    }

    path = build_storage_path(key);
    if (!path)
    {
        return errno ? errno : ENOMEM;
    }

    status = read_file_all(path, &data, &len);
    free(path);
    if (status != 0)
    {
        return status;
    }

    *value = malloc(len + 1);
    if (!*value)
    {
        free(data);
        return ENOMEM;
    }

    memcpy(*value, data, len);
    (*value)[len] = '\0';
    free(data);
    return 0;
}

int secure_store_write_seckey(const uint8_t seckey[32])
{
    char *dir;
    char *path;
    int status;

    if (!seckey)
    {
        return EINVAL;
    }

    dir = sequence_storage_dir_dup();
    if (!dir)
    {
        return ENOMEM;
    }

    status = ensure_storage_dir(dir);
    free(dir);
    if (status != 0)
    {
        return status;
    }

    path = build_storage_path("seckey");
    if (!path)
    {
        return errno ? errno : ENOMEM;
    }

    status = write_file_atomic(path, seckey, 32);
    free(path);
    return status;
}

int secure_store_read_seckey(uint8_t seckey[32])
{
    char *path;
    uint8_t *data = NULL;
    size_t len = 0;
    int status;

    if (!seckey)
    {
        return EINVAL;
    }

    path = build_storage_path("seckey");
    if (!path)
    {
        return errno ? errno : ENOMEM;
    }

    status = read_file_all(path, &data, &len);
    free(path);
    if (status != 0)
    {
        return status;
    }

    if (len != 32)
    {
        free(data);
        return EINVAL;
    }

    memcpy(seckey, data, 32);
    free(data);
    return 0;
}
