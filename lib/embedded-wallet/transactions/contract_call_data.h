#include <stddef.h>
#include <stdint.h>

typedef enum ArgType {
    ARG_BOOL,
    ARG_I64,
    ARG_U64,
    ARG_F64,
    ARG_STRING,   // UTF-8, NUL-terminated
    ARG_BYTES     // arbitrary bytes
} ArgType;

typedef struct BytesView {
    const uint8_t *data;
    size_t len;
} BytesView;

typedef struct Arg {
    ArgType type;
    union {
        int boolean;          // 0/1
        int64_t i64;
        uint64_t u64;
        double f64;
        const char *str;      // NUL-terminated
        BytesView bytes;      // pointer + length
    } v;
} Arg;

typedef struct SequenceContractCallData {
    char *contract_address;
    char *function_signature;
    char *value_wei;
    const Arg *args;
    size_t args_len;
} SequenceContractCallData;
