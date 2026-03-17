#include <stdio.h>
#include <string.h>
#include "lib/indexer/get_token_balances.h"
#include "lib/wallet/sequence_config.h"
#include "lib/wallet/sequence_connector.h"

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

    char *access_key = "AQAAAAAAAKMyIkcpH4HUX6fFHcyNnjjSrak";
    sequence_config_init(access_key);

    // **
    // INDEXER
    // **

    const char *chain_id = "137";
    const char *contract_address = "0x3c499c542cEF5E3811e1192ce70d8cC03d5c3359";
    const char *wallet_address = "0x8e3E38fe7367dd3b52D1e281E4e8400447C8d8B9";

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
    sequence_complete_auth_return *response = sequence_confirm_email_sign_in(email, code);

    sequence_wallet *wallet;
    if (response->wallet_count == 0)
    {
        wallet = sequence_create_wallet();
    }
    else
    {
        wallet = sequence_use_wallet(response->wallets[0].type);
    }

    sequence_complete_auth_return_free(response);

    printf("Wallet address: %s\n", wallet->address);

    // **
    // SIGN MESSAGE
    // **

    READ_LINE("Enter message to sign: ", message);
    const char *signature = sequence_sign_message("80002", message);
    printf("Signature from '%s': %s\n", message, signature);

    // **
    // SEND TRANSACTION
    // **

    const char *to = "0xE5E8B483FfC05967FcFed58cc98D053265af6D99";
    const char *value = "1000";

    const char *hash = sequence_send_transaction("80002", to, value);
    printf("Transaction hash: %s\n", hash);

    return 0;
}