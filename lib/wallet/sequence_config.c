#include "sequence_config.h"

sequence_config_t sequence_config = {
    .access_key = ""
};

void sequence_config_init(const char *new_access_key) {
    sequence_config.access_key = new_access_key;
}
