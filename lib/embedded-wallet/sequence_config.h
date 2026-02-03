typedef struct {
    const char *access_key;
} sequence_config_t;

extern sequence_config_t sequence_config;

void sequence_config_init(const char *new_access_key);