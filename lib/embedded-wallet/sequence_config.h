#ifndef SEQUENCE_CONFIG_H
#define SEQUENCE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        const char *access_key;
    } sequence_config_t;

    /*
     * Public, process-wide config object.
     * Define exactly once in sequence_config.c.
     */
    extern sequence_config_t sequence_config;

    void sequence_config_init(const char *new_access_key);

#ifdef __cplusplus
}
#endif

#endif /* SEQUENCE_CONFIG_H */