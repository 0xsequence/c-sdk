#include "embedded-wallet/sequence_wallet.h"

#include <stdint.h>
#include <stddef.h>

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

typedef struct SequenceContractCallResult {
    uint8_t *payload;     // owned by caller (or by callee; define clearly)
    size_t payload_len;

    int status;           // 0 = ok, nonzero = error
    char error[256];      // optional human-readable error
} SequenceContractCallResult;

char *sequence_contract_call(
	sequence_wallet_t *wallet,
    uint64_t chain_id,
    const char *contract_address,
    uint64_t value_wei,
    const char *function_signature
);
