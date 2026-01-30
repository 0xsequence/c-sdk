#include "timestamps.h"
#include <time.h>

long timestamp_now_seconds(void) {
    return (long)time(NULL);
}

long timestamp_seconds_from_now(const long seconds_from_now) {
    const time_t now = time(NULL);

    if (now == (time_t)-1) {
        return -1;
    }

    return now + seconds_from_now;
}
