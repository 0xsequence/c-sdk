#include <stdint.h>

char *sequence_read_file(const char *path);
uint32_t sequence_write_file(const char *path, char *data, uint32_t data_size);
