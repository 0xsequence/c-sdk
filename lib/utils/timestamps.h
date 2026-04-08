#ifndef SEQUENCE_TIMESTAMPS_H
#define SEQUENCE_TIMESTAMPS_H

long timestamp_now_seconds();

long long timestamp_now_milliseconds();

long long timestamp_next_nonce();

long timestamp_seconds_from_now(long seconds_from_now);

#endif
