#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/infrastructure/is_valid_message_signature.h"
#include "lib/indexer/get_token_balances.h"
#include "lib/wallet/oms_wallet_config.h"
#include "lib/wallet/oms_wallet.h"

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

    char *access_key = "AQAAAAAAAAK2JvvZhWqZ51riasWBftkrVXE";
    REQUIRE(oms_wallet_config_init(access_key) == 0, "oms_wallet_config_init failed");
    REQUIRE(
        oms_wallet_config_set_indexer_url_template("https://dev-{value}-indexer.sequence.app/rpc/Indexer/") == 0,
        "oms_wallet_config_set_indexer_url_template failed");
    REQUIRE(
        oms_wallet_config_set_api_rpc_url("https://dev-api.sequence.app/rpc/API") == 0,
        "oms_wallet_config_set_api_rpc_url failed");

    // **
    // INDEXER
    // **

    const char *chain_id = "137";
    const char *contract_address = "0x3c499c542cEF5E3811e1192ce70d8cC03d5c3359";
    const char *wallet_address = "0x8e3E38fe7367dd3b52D1e281E4e8400447C8d8B9";

    OmsWalletGetTokenBalancesReturn *tokenBalances = oms_wallet_get_token_balances(
        chain_id,
        contract_address,
        wallet_address,
        true);

    log_oms_wallet_get_token_balances_return(tokenBalances);
    oms_wallet_free_token_balances_return(tokenBalances);

    // **
    // EMAIL AUTHENTICATION
    // **

    char email[256];
    char code[64];
    char message[256];

    READ_LINE("Enter email: ", email);
    REQUIRE(oms_wallet_start_email_sign_in(email), "start_email_sign_in failed");

    READ_LINE("Enter code: ", code);
    oms_wallet_complete_auth_response_t *response = oms_wallet_complete_email_sign_in(code);
    REQUIRE(response, "complete_email_sign_in failed");

    const char *target_wallet_type = oms_wallet_default_wallet_type();
    oms_wallet_t *wallet = NULL;

    for (size_t i = 0; response->complete_auth_response && i < response->complete_auth_response->wallets.count; i++)
    {
        if (response->complete_auth_response->wallets.items[i] &&
            strcmp(
                waas_wallet_type_to_string(response->complete_auth_response->wallets.items[i]->type),
                target_wallet_type) == 0)
        {
            wallet = oms_wallet_use_wallet(response->complete_auth_response->wallets.items[i]->id);
            break;
        }
    }

    if (!wallet)
    {
        wallet = oms_wallet_create_wallet_of_type(target_wallet_type);
    }
    REQUIRE(wallet, "wallet selection failed");

    oms_wallet_free_complete_auth(response);

    printf("Wallet address: %s\n", wallet->address);

    // **
    // SIGN MESSAGE
    // **

    READ_LINE("Enter message to sign: ", message);
    oms_wallet_sign_message_response_t *signature = oms_wallet_sign_message("80002", message);
    REQUIRE(signature, "sign_message failed");
    printf(
        "Signature from '%s': %s\n",
        message,
        (signature && signature->sign_message_response) ? signature->sign_message_response->signature : "(null)");

    OmsWalletIsValidMessageSignatureReturn *is_valid_message_signature = oms_wallet_is_valid_message_signature(
        "80002",
        wallet->address,
        message,
        (signature && signature->sign_message_response) ? signature->sign_message_response->signature : NULL);
    printf("Signature valid: %s (status=%d)\n",
        (is_valid_message_signature && is_valid_message_signature->is_valid) ? "true" : "false",
        is_valid_message_signature ? is_valid_message_signature->status : -1);
    oms_wallet_free_is_valid_message_signature_return(is_valid_message_signature);
    if (signature) {
        oms_wallet_free_sign_message(signature);
    }

    // **
    // SEND TRANSACTION
    // **

    const char *to = "0xE5E8B483FfC05967FcFed58cc98D053265af6D99";
    const char *value = "0";

    oms_wallet_send_transaction_response_t *tx = oms_wallet_send_transaction("80002", to, value);
    REQUIRE(tx, "send_transaction failed");
    printf(
        "Transaction hash: %s\n",
        (tx && tx->send_transaction_response) ? tx->send_transaction_response->tx_hash : "(null)");
    if (tx) {
        oms_wallet_free_send_transaction(tx);
    }
    oms_wallet_free(wallet);

    return 0;
}
