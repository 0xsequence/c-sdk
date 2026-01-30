#include "storage_handler.h"

char *sequence_read_file(const char *path)
{
    /*MMIFILE_HANDLE fd;
    MMIFILE_ERROR_E err;
    unsigned int mode = SFS_MODE_READ | SFS_MODE_OPEN_EXISTING;
    int size;
    int offset = 0;
    char* data = NULL;
    fd = MMIFILE_CreateFile(path, mode, NULL, NULL);
    if (fd == SFS_INVALID_HANDLE) {
        return NULL;
    }
    size = MMIFILE_GetFileSize(fd);
    data = malloc(size + 1);
    SANITY_CHECK(data == NULL, "alloc memory failed", failed);
    data[size] = '\0';
    while (offset < size) {
        int temp = 0;
        err = MMIFILE_ReadFile(fd, data + offset, size - offset, &temp, NULL);
        if (err != 0) {
            dbg("read file failed, %d", err);
            MMIFILE_CloseFile(fd);
            goto failed;
        }
        offset += temp;
    }
    MMIFILE_CloseFile(fd);
    return data;
    failed:
    if (data != NULL) free(data);
    data = NULL;
    return data;*/
    return "";
}

uint32_t sequence_write_file(const char *path, char *data, uint32_t data_size)
{
    /*
    MMIFILE_HANDLE fd;
    unsigned int mode = SFS_MODE_CREATE_NEW | SFS_MODE_WRITE |SFS_MODE_READ;

    fd = MMIFILE_CreateFile(path, mode, NULL, NULL);

    _Write(fd, test_public_key, strlen(test_public_key));

    int offset = 0;
    MMIFILE_ERROR_E err;
    while (data_size > 0) {
        int temp = 0;
        err = MMIFILE_WriteFile(fd, &data[offset],
        data_size, &temp, NULL);
        if (err != 0) {
            dbg("write failed! %d", err);
            return -1;
        }
        offset += temp;
        data_size -= temp;
    }

    // Cleanup
    MMIFILE_CloseFile(fd);
    fd = SFS_INVALID_HANDLE;

    return 0;
    */
    return -1;
}
