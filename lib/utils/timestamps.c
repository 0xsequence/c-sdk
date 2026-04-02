#include "timestamps.h"
#include <time.h>

long timestamp_now_seconds(void) {
    return (long)time(NULL);
}

long long timestamp_now_milliseconds(void) {
    struct timespec ts;

    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return -1;
    }

    return ((long long)ts.tv_sec * 1000LL) + ((long long)ts.tv_nsec / 1000000LL);
}

long long timestamp_next_nonce(void) {
    static long long last_nonce = 0;
    const long long now_ms = timestamp_now_milliseconds();

    if (now_ms < 0) {
        return -1;
    }

    if (now_ms <= last_nonce) {
        last_nonce += 1;
    } else {
        last_nonce = now_ms;
    }

    return last_nonce;
}

long timestamp_seconds_from_now(const long seconds_from_now) {
    const time_t now = time(NULL);

    if (now == (time_t)-1) {
        return -1;
    }

    return now + seconds_from_now;
}
