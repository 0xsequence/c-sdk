#include <stdio.h>
#include <string.h>
#include "lib/indexer/get_token_balances.h"
#include "lib/embedded-wallet/sequence_config.h"
#include "lib/embedded-wallet/sequence_login.h"
#include "lib/embedded-wallet/transactions/contract_call.h"

static void strip_newline(char *s) {
    if (!s)
        return;

    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[n - 1] = '\0';
        n--;
    }
}

int main(void) {
    // **
    // SETUP
    // **

    char *access_key = "oesk7yu5tjNfQElu5HjuUunAAAAAAAAAA";
    sequence_config_init(access_key);

    // **
    // INDEXER
    // **

    uint64_t chain_id = 137;
    char *contract_address = "0x3c499c542cEF5E3811e1192ce70d8cC03d5c3359";
    char *wallet_address = "0x8e3E38fe7367dd3b52D1e281E4e8400447C8d8B9";

    SequenceGetTokenBalancesReturn *tokenBalances = sequence_get_token_balances(
        chain_id,
        contract_address,
        wallet_address,
        true);

    log_sequence_get_token_balances_return(tokenBalances);
    free_sequence_token_balances_return(tokenBalances);

    // **
    // EMAIL AUTHENTICATION
    // **

    char email[256];
    char code[64];

    printf("Enter email: ");
    if (!fgets(email, sizeof(email), stdin)) {
        fprintf(stderr, "Failed to read email\n");
        return 1;
    }
    strip_newline(email);

    if (!sign_in_with_email(email)) {
        fprintf(stderr, "sign_in_with_email failed\n");
        return 1;
    }

    printf("Enter code: ");
    if (!fgets(code, sizeof(code), stdin)) {
        fprintf(stderr, "Failed to read code\n");
        return 1;
    }
    strip_newline(code);

    sequence_wallet_t *wallet = confirm_email_sign_in(email, code);

    printf("Wallet address: %s\n", wallet->address);
    printf("Session id: %s\n", wallet->session_id);

    // **
    // CONTRACT CALL
    // **

    uint64_t chain_id_mint = 80002;
    char *contract_address_mint = "0x7e3DA4a3bC319962EF5CA37B05aD107Fc03cFBd6";
    char *function_signature = "mint(address,uint256)";
    char *address_arg = "0x7e3DA4a3bC319962EF5CA37B05aD107Fc03cFBd6";
    char *uint256_arg = "123456789012345678901234567890123456789012345678901234567890";

    Arg args[2];

    args[0].type = ARG_STRING;
    args[0].v.str = address_arg;

    args[1].type = ARG_STRING;
    args[1].v.str = uint256_arg;

    char *hash = sequence_contract_call(
        wallet,
        chain_id_mint,
        contract_address_mint,
        0,
        function_signature,
        args, 2);

    printf("Transaction hash: %s\n", hash);

    return 0;
}