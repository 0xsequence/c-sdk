#ifndef OMS_WALLET_SECURE_STORAGE_H
#define OMS_WALLET_SECURE_STORAGE_H

#include <stdint.h>
#include <stddef.h>

int secure_store_write_string(const char *key, const char *value);

int secure_store_read_string(const char *key, char **value);

int secure_store_delete(const char *key);

int secure_store_write_string_at(const char *storage_dir, const char *key, const char *value);

int secure_store_read_string_at(const char *storage_dir, const char *key, char **value);

int secure_store_delete_at(const char *storage_dir, const char *key);

int secure_store_write_seckey(const uint8_t seckey[32]);

int secure_store_read_seckey(uint8_t seckey[32]);

int secure_store_delete_seckey(void);

int secure_store_write_seckey_at(const char *storage_dir, const uint8_t seckey[32]);

int secure_store_read_seckey_at(const char *storage_dir, uint8_t seckey[32]);

int secure_store_delete_seckey_at(const char *storage_dir);

int secure_store_status_is_not_found(int status);

#endif
