#include <stdint.h>
#include <stddef.h>

int secure_store_write_access_key(const char *access_key);

int secure_store_read_access_key(char **access_key);

int secure_store_write_challenge(const char *challenge);

int secure_store_read_challenge(char **challenge);

int secure_store_write_seckey(const uint8_t seckey[32]);

int secure_store_read_seckey(uint8_t seckey[32]);
