// demo_sequence_get_token_balances.c
// C99 demo file showing how you might call:
//   SequenceTokenBalances sequence_get_token_balances(uint64_t chain_id,
//       const char *contract_address, const char *wallet_address,
//       const char *includeMetadata);
//
// This is a DEMO with stub types + a stub implementation so it can compile/run.
// Replace the stub implementation with your real integration.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// ---------------------------
// Demo types (replace as needed)
// ---------------------------

typedef struct SequenceTokenBalanceItem {
    const char *token_address;     // e.g. ERC-20 contract address
    const char *balance_decimal;   // e.g. "1000000000000000000"
    const char *symbol;            // optional metadata
    const char *name;              // optional metadata
    uint32_t decimals;             // optional metadata
} SequenceTokenBalanceItem;

typedef struct SequenceTokenBalances {
    int status;                    // 0 = ok, nonzero = error
    char error[256];               // error message if status != 0

    SequenceTokenBalanceItem *items;
    size_t item_count;

    // In a real implementation, you might also include pagination info, etc.
} SequenceTokenBalances;

// ---------------------------
// Function declaration (as requested)
// ---------------------------

SequenceTokenBalances sequence_get_token_balances(
    uint64_t chain_id,
    const char *contract_address,
    const char *wallet_address,
    bool include_metadata
);

// ---------------------------
// Demo stub implementation (replace with real integration)
// ---------------------------

SequenceTokenBalances sequence_get_token_balances(
    uint64_t chain_id,
    const char *contract_address,
    const char *wallet_address,
    bool include_metadata
) {
    (void)chain_id;
    (void)contract_address;
    (void)wallet_address;
    (void)includeMetadata;

    SequenceTokenBalances out;
    out.status = -1;
    memset(out.error, 0, sizeof(out.error));
    strncpy(out.error, "sequence_get_token_balances not implemented", sizeof(out.error) - 1);
    out.items = NULL;
    out.item_count = 0;
    return out;
}

// Optional: a cleanup hook (if your real impl allocates memory)
static void sequence_token_balances_free(SequenceTokenBalances *b) {
    // If your real implementation allocates b->items and/or strings, free them here.
    // For this demo stub, nothing to free.
    (void)b;
}

// ---------------------------
// Demo usage
// ---------------------------

int main(void) {
    uint64_t chain_id = 421614; // Arbitrum Sepolia (example)
    const char *contract_address = "0x05F9f33a358a66aea1a7479d001cE8f64450eA39"; // token contract (or NULL for "all", depending on your API)
    const char *wallet_address   = "0xE5E8B483FfC05967FcFed58cc98D053265af6D99";
    const char *includeMetadata  = "true"; // could be "true"/"false", "1"/"0", or NULL—depends on your API

    SequenceTokenBalances balances = sequence_get_token_balances(
        chain_id,
        contract_address,
        wallet_address,
        includeMetadata
    );

    if (balances.status != 0) {
        printf("Error: %s\n", balances.error);
        sequence_token_balances_free(&balances);
        return 1;
    }

    printf("Token balances (%zu):\n", balances.item_count);
    for (size_t i = 0; i < balances.item_count; i++) {
        const SequenceTokenBalanceItem *it = &balances.items[i];
        printf("  - token: %s\n", it->token_address ? it->token_address : "(null)");
        printf("    balance: %s\n", it->balance_decimal ? it->balance_decimal : "(null)");
        if (it->symbol)   printf("    symbol: %s\n", it->symbol);
        if (it->name)     printf("    name: %s\n", it->name);
        if (it->decimals) printf("    decimals: %u\n", it->decimals);
    }

    sequence_token_balances_free(&balances);
    return 0;
}
