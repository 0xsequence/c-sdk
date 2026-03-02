#include <stdio.h>
#include <string.h>
#include "lib/indexer/get_token_balances.h"
#include "lib/embedded-wallet/sequence_config.h"
#include "lib/embedded-wallet/sequence_login.h"
#include "lib/embedded-wallet/transactions/contract_call.h"

#define READ_LINE(prompt, buf) do {                                   \
    printf("%s", (prompt));                                           \
    if (!fgets((buf), sizeof(buf), stdin)) {                          \
        fprintf(stderr, "Failed to read input\n");                    \
        return 1;                                                     \
    }                                                                 \
    strip_newline((buf));                                             \
} while (0)

#define REQUIRE(ok, msg) do {                                         \
    if (!(ok)) {                                                      \
        fprintf(stderr, "%s\n", (msg));                               \
        return 1;                                                     \
    }                                                                 \
} while (0)

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

    char *access_key = "AQAAAAAAAAXVnbE_7r6-4StU84lwV9oVcKA";
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
    char message[256];

    READ_LINE("Enter email: ", email);
    REQUIRE(sequence_sign_in_with_email(email), "sign_in_with_email failed");

    READ_LINE("Enter code: ", code);
    const sequence_complete_auth_return response = sequence_confirm_email_sign_in(email, code);

    sequence_wallet *wallet;
    if (response.wallet_count == 0)
    {
        wallet = sequence_create_wallet("Ethereum_SequenceV3");
    }
    else
    {
        wallet = sequence_use_wallet(response.wallets[0].type);
    }

    printf("Wallet address: %s\n", wallet->address);

    // **
    // SIGN MESSAGE
    // **

    READ_LINE("Enter message to sign: ", message);
    const char *signature = sequence_sign_message("amoy", message);
    printf("Signature from '%s': %s\n", message, signature);

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

    /*char *hash = sequence_contract_call(
        wallet,
        chain_id_mint,
        contract_address_mint,
        0,
        function_signature,
        args, 2);

    printf("Transaction hash: %s\n", hash);*/

    return 0;
}