#include "secure_storage.h"

#include <Security/Security.h>
#include <CoreFoundation/CoreFoundation.h>

#include <string.h>
#include <stdlib.h>

#define SERVICE_NAME "io.omswallet.c-sdk.service"

/* -------------------------------------------------- */
/* Internal helpers                                  */
/* -------------------------------------------------- */

static CFStringRef cfstr(const char *cstr)
{
    return CFStringCreateWithCString(
        NULL, cstr, kCFStringEncodingUTF8);
}

/* Write arbitrary bytes */
static OSStatus keychain_write(
    const char *account,
    const void *data,
    size_t data_len)
{
    CFStringRef service = cfstr(SERVICE_NAME);
    CFStringRef account_cf = cfstr(account);

    const void *query_keys[] = {
        kSecClass,
        kSecAttrService,
        kSecAttrAccount
    };

    const void *query_vals[] = {
        kSecClassGenericPassword,
        service,
        account_cf
    };

    CFDictionaryRef query =
        CFDictionaryCreate(NULL,
                           query_keys,
                           query_vals,
                           3,
                           &kCFTypeDictionaryKeyCallBacks,
                           &kCFTypeDictionaryValueCallBacks);

    /* delete existing */
    SecItemDelete(query);
    CFRelease(query);

    CFDataRef value =
        CFDataCreate(NULL,
                     (const UInt8 *)data,
                     data_len);

    const void *add_keys[] = {
        kSecClass,
        kSecAttrService,
        kSecAttrAccount,
        kSecValueData
    };

    const void *add_vals[] = {
        kSecClassGenericPassword,
        service,
        account_cf,
        value
    };

    CFDictionaryRef add =
        CFDictionaryCreate(NULL,
                           add_keys,
                           add_vals,
                           4,
                           &kCFTypeDictionaryKeyCallBacks,
                           &kCFTypeDictionaryValueCallBacks);

    OSStatus status = SecItemAdd(add, NULL);

    CFRelease(add);
    CFRelease(value);
    CFRelease(service);
    CFRelease(account_cf);

    return status;
}

/* Read arbitrary bytes */
static OSStatus keychain_read(
    const char *account,
    uint8_t **out_data,
    size_t *out_len)
{
    CFStringRef service = cfstr(SERVICE_NAME);
    CFStringRef account_cf = cfstr(account);

    const void *keys[] = {
        kSecClass,
        kSecAttrService,
        kSecAttrAccount,
        kSecReturnData,
        kSecMatchLimit
    };

    const void *vals[] = {
        kSecClassGenericPassword,
        service,
        account_cf,
        kCFBooleanTrue,
        kSecMatchLimitOne
    };

    CFDictionaryRef query =
        CFDictionaryCreate(NULL,
                           keys,
                           vals,
                           5,
                           &kCFTypeDictionaryKeyCallBacks,
                           &kCFTypeDictionaryValueCallBacks);

    CFTypeRef result = NULL;
    OSStatus status = SecItemCopyMatching(query, &result);

    if (status == errSecSuccess) {
        CFDataRef data = (CFDataRef)result;

        *out_len = CFDataGetLength(data);
        *out_data = malloc(*out_len);

        memcpy(*out_data,
               CFDataGetBytePtr(data),
               *out_len);

        CFRelease(data);
    }

    CFRelease(service);
    CFRelease(account_cf);
    CFRelease(query);

    return status;
}

static OSStatus keychain_delete(const char *account)
{
    CFStringRef service = cfstr(SERVICE_NAME);
    CFStringRef account_cf = cfstr(account);
    OSStatus status;

    const void *query_keys[] = {
        kSecClass,
        kSecAttrService,
        kSecAttrAccount
    };

    const void *query_vals[] = {
        kSecClassGenericPassword,
        service,
        account_cf
    };

    CFDictionaryRef query =
        CFDictionaryCreate(NULL,
                           query_keys,
                           query_vals,
                           3,
                           &kCFTypeDictionaryKeyCallBacks,
                           &kCFTypeDictionaryValueCallBacks);

    status = SecItemDelete(query);

    CFRelease(service);
    CFRelease(account_cf);
    CFRelease(query);

    if (status == errSecItemNotFound)
    {
        return errSecSuccess;
    }

    return status;
}

/* -------------------------------------------------- */
/* Public API                                        */
/* -------------------------------------------------- */

int secure_store_write_string(const char *key, const char *value)
{
    if (!key || !value)
        return errSecParam;

    return keychain_write(
        key,
        value,
        strlen(value));
}

int secure_store_read_string(const char *key, char **value)
{
    if (!key || !value)
        return errSecParam;

    uint8_t *data = NULL;
    size_t len = 0;

    OSStatus status =
        keychain_read(key, &data, &len);

    if (status != errSecSuccess)
        return status;

    *value = malloc(len + 1);
    if (!*value) {
        free(data);
        return errSecAllocate;
    }

    memcpy(*value, data, len);
    (*value)[len] = '\0';

    free(data);
    return errSecSuccess;
}

int secure_store_delete(const char *key)
{
    if (!key)
        return errSecParam;

    return keychain_delete(key);
}

int secure_store_write_seckey(const uint8_t seckey[32])
{
    return keychain_write("seckey", seckey, 32);
}

int secure_store_read_seckey(uint8_t seckey[32])
{
    uint8_t *data = NULL;
    size_t len = 0;

    OSStatus status =
        keychain_read("seckey", &data, &len);

    if (status != errSecSuccess) {
        free(data);
        return status;
    }

    if (len != 32) {
        free(data);
        return errSecDecode;
    }

    memcpy(seckey, data, 32);
    free(data);

    return errSecSuccess;
}

int secure_store_delete_seckey(void)
{
    return keychain_delete("seckey");
}

int secure_store_status_is_not_found(int status)
{
    return status == errSecItemNotFound;
}
