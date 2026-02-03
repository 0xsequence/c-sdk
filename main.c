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
    sequence_config_init("oesk7yu5tjNfQElu5HjuUunAAAAAAAAAA");

    SequenceGetTokenBalancesReturn *tokenBalances = sequence_get_token_balances(
        137,
        "0x3c499c542cEF5E3811e1192ce70d8cC03d5c3359",
        "0x8e3E38fe7367dd3b52D1e281E4e8400447C8d8B9",
        true);

    log_sequence_get_token_balances_return(tokenBalances);
    free_sequence_token_balances_return(tokenBalances);

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

    printf("wallet address: %s\n", wallet->address);
    printf("session id: %s\n", wallet->session_id);

    Arg args[2];

    args[0].type = ARG_STRING;
    args[0].v.str = "0x7e3DA4a3bC319962EF5CA37B05aD107Fc03cFBd6";

    args[1].type = ARG_STRING;
    args[1].v.str = "123456789012345678901234567890123456789012345678901234567890";

    char *hash = sequence_contract_call(
        wallet,
        80002,
        "0x7e3DA4a3bC319962EF5CA37B05aD107Fc03cFBd6",
        0,
        "mint(address,uint256)",
        args, 2);

    printf("hash: %s\n", hash);

    return 0;
}