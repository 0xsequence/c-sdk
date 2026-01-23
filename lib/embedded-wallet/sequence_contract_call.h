#ifndef SEQUENCE_CONTRACT_CALL_H
#define SEQUENCE_CONTRACT_CALL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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

    /**
     * Example result container.
     * You can swap this to whatever your "sequence" layer needs (ABI-encoded calldata,
     * tx JSON, raw bytes, etc.).
     */
    typedef struct SequenceContractCallResult {
        uint8_t *payload;     // owned by caller (or by callee; define clearly)
        size_t payload_len;

        int status;           // 0 = ok, nonzero = error
        char error[256];      // optional human-readable error
    } SequenceContractCallResult;

    /**
     * sequenceContractCall(chainId, contractAddress, value, functionSignature, args)
     *
     * - chain_id: EVM chain id
     * - contract_address: "0x..." string (recommended: checksum not required, but validate length)
     * - value_wei: amount in wei (fits in u64 only for smaller values; consider uint256 if needed)
     * - function_signature: e.g. "transfer(address,uint256)"
     * - args/arg_count: heterogenous arguments, tagged by ArgType
     *
     * Returns a SequenceCallResult; status==0 indicates success.
     */
    SequenceContractCallResult sequence_contract_call(
        uint64_t chain_id,
        const char *contract_address,
        uint64_t value_wei,
        const char *function_signature,
        const Arg *args,
        size_t arg_count
    );

#ifdef __cplusplus
}
#endif

#endif // SEQUENCE_CONTRACT_CALL_H
