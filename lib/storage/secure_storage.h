#include <stdint.h>
#include <stddef.h>

int secure_store_write_string(const char *key, const char *value);

int secure_store_read_string(const char *key, char **value);

int secure_store_write_seckey(const uint8_t seckey[32]);

int secure_store_read_seckey(uint8_t seckey[32]);
