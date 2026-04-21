#include "secure_storage.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "wallet/sequence_config.h"

#define SEQUENCE_DEFAULT_STORAGE_DIR ".sequence-c-sdk"
#define SEQUENCE_TMPFILE_MAX_ATTEMPTS 128

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

static int open_storage_dir(int *out_fd)
{
    char *dir = NULL;
    int dir_fd = -1;
    int status = 0;
    struct stat st;

    if (!out_fd)
    {
        return EINVAL;
    }

    *out_fd = -1;
    dir = sequence_storage_dir_dup();
    if (!dir)
    {
        return ENOMEM;
    }

    if (mkdir(dir, 0700) != 0 && errno != EEXIST)
    {
        status = errno;
        goto cleanup;
    }

    dir_fd = open(
        dir,
        O_RDONLY | O_DIRECTORY | O_CLOEXEC
#ifdef O_NOFOLLOW
        | O_NOFOLLOW
#endif
    );
    if (dir_fd < 0)
    {
        status = errno;
        goto cleanup;
    }

    if (fstat(dir_fd, &st) != 0)
    {
        status = errno;
        goto cleanup;
    }

    if (!S_ISDIR(st.st_mode))
    {
        status = ENOTDIR;
        goto cleanup;
    }

    if ((st.st_mode & 0777) != 0700 && fchmod(dir_fd, 0700) != 0)
    {
        status = errno;
        goto cleanup;
    }

    *out_fd = dir_fd;
    dir_fd = -1;

cleanup:
    free(dir);
    if (dir_fd >= 0)
    {
        close(dir_fd);
    }
    return status;
}

static char *build_storage_filename(const char *key)
{
    char *filename;
    size_t key_len;

    if (!key || key[0] == '\0')
    {
        return NULL;
    }

    key_len = strlen(key);
    filename = malloc(key_len + 1);
    if (!filename)
    {
        return NULL;
    }

    for (size_t i = 0; i < key_len; ++i)
    {
        filename[i] = sanitize_key_char(key[i]);
    }
    filename[key_len] = '\0';
    return filename;
}

static int write_file_atomic(int dir_fd, const char *filename, const void *data, size_t len)
{
    char *tmp_name = NULL;
    int fd = -1;
    int status = 0;
    size_t total = 0;
    int created_tmp = 0;

    if (dir_fd < 0 || !filename || (!data && len != 0))
    {
        return EINVAL;
    }

    {
        size_t tmp_name_len = strlen(filename) + 32;

        tmp_name = malloc(tmp_name_len);
        if (!tmp_name)
        {
            return ENOMEM;
        }

        for (unsigned int attempt = 0; attempt < SEQUENCE_TMPFILE_MAX_ATTEMPTS; ++attempt)
        {
            snprintf(
                tmp_name,
                tmp_name_len,
                ".%s.tmp.%ld.%u",
                filename,
                (long)getpid(),
                attempt);

            fd = openat(
                dir_fd,
                tmp_name,
                O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC,
                0600);
            if (fd >= 0)
            {
                created_tmp = 1;
                break;
            }

            if (errno != EEXIST)
            {
                status = errno;
                goto cleanup;
            }
        }
    }

    if (fd < 0)
    {
        status = EEXIST;
        goto cleanup;
    }

    while (total < len)
    {
        ssize_t written = write(fd, (const uint8_t *)data + total, len - total);

        if (written < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            status = errno;
            goto cleanup;
        }
        if (written == 0)
        {
            status = EIO;
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
        status = errno;
        fd = -1;
        goto cleanup;
    }
    fd = -1;

    if (renameat(dir_fd, tmp_name, dir_fd, filename) != 0)
    {
        status = errno;
        goto cleanup;
    }

    created_tmp = 0;
    if (fsync(dir_fd) != 0)
    {
        status = errno;
        goto cleanup;
    }

cleanup:
    if (fd >= 0)
    {
        close(fd);
    }
    if (created_tmp)
    {
        unlinkat(dir_fd, tmp_name, 0);
    }
    free(tmp_name);
    return status;
}

static int read_file_all(int dir_fd, const char *filename, uint8_t **out_data, size_t *out_len)
{
    struct stat st;
    int fd = -1;
    uint8_t *data = NULL;
    size_t total = 0;

    if (dir_fd < 0 || !filename || !out_data || !out_len)
    {
        return EINVAL;
    }

    *out_data = NULL;
    *out_len = 0;

    fd = openat(
        dir_fd,
        filename,
        O_RDONLY | O_CLOEXEC
#ifdef O_NOFOLLOW
        | O_NOFOLLOW
#endif
    );
    if (fd < 0)
    {
        return errno;
    }

    if (fstat(fd, &st) != 0)
    {
        int status = errno;

        close(fd);
        return status;
    }

    if (!S_ISREG(st.st_mode) || st.st_size < 0)
    {
        close(fd);
        return EINVAL;
    }

    if (st.st_size > 0)
    {
        data = malloc((size_t)st.st_size);
        if (!data)
        {
            close(fd);
            return ENOMEM;
        }
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
            free(data);
            close(fd);
            return EIO;
        }

        total += (size_t)nread;
    }

    if (close(fd) != 0)
    {
        free(data);
        return errno;
    }
    *out_data = data;
    *out_len = total;
    return 0;
}

static int secure_store_write_bytes(const char *key, const void *data, size_t len)
{
    char *filename = NULL;
    int dir_fd = -1;
    int status;

    if (!key || (!data && len != 0))
    {
        return EINVAL;
    }

    filename = build_storage_filename(key);
    if (!filename)
    {
        return ENOMEM;
    }

    status = open_storage_dir(&dir_fd);
    if (status != 0)
    {
        free(filename);
        return status;
    }

    status = write_file_atomic(dir_fd, filename, data, len);
    close(dir_fd);
    free(filename);
    return status;
}

static int secure_store_read_bytes(const char *key, uint8_t **out_data, size_t *out_len)
{
    char *filename = NULL;
    int dir_fd = -1;
    int status;

    if (!key || !out_data || !out_len)
    {
        return EINVAL;
    }

    filename = build_storage_filename(key);
    if (!filename)
    {
        return ENOMEM;
    }

    status = open_storage_dir(&dir_fd);
    if (status != 0)
    {
        free(filename);
        return status;
    }

    status = read_file_all(dir_fd, filename, out_data, out_len);
    close(dir_fd);
    free(filename);
    return status;
}

int secure_store_write_string(const char *key, const char *value)
{
    if (!key || !value)
    {
        return EINVAL;
    }

    return secure_store_write_bytes(key, value, strlen(value));
}

int secure_store_read_string(const char *key, char **value)
{
    uint8_t *data = NULL;
    size_t len = 0;
    int status;

    if (!key || !value)
    {
        return EINVAL;
    }

    status = secure_store_read_bytes(key, &data, &len);
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

    if (len > 0)
    {
        memcpy(*value, data, len);
    }
    (*value)[len] = '\0';
    free(data);
    return 0;
}

int secure_store_write_seckey(const uint8_t seckey[32])
{
    if (!seckey)
    {
        return EINVAL;
    }

    return secure_store_write_bytes("seckey", seckey, 32);
}

int secure_store_read_seckey(uint8_t seckey[32])
{
    uint8_t *data = NULL;
    size_t len = 0;
    int status;

    if (!seckey)
    {
        return EINVAL;
    }

    status = secure_store_read_bytes("seckey", &data, &len);
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
