#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/infrastructure/is_valid_message_signature.h"
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

    char *access_key = "AQAAAAAAAAK2JvvZhWqZ51riasWBftkrVXE";
    sequence_config_init(access_key);
    sequence_config_set_indexer_url_template("https://dev-{value}-indexer.sequence.app/rpc/Indexer/");
    sequence_config_set_api_rpc_url("https://dev-api.sequence.app/rpc/API");

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
    waas_wallet_complete_auth_response *response = sequence_confirm_email_sign_in(code);
    REQUIRE(response, "confirm_email_sign_in failed");

    const char *target_wallet_type = sequence_default_wallet_type();
    waas_wallet *wallet = NULL;

    for (size_t i = 0; i < response->wallets.count; i++)
    {
        if (response->wallets.items[i] &&
            strcmp(
                waas_wallet_type_to_string(response->wallets.items[i]->type),
                target_wallet_type) == 0)
        {
            wallet = sequence_use_wallet(target_wallet_type);
            break;
        }
    }

    if (!wallet)
    {
        wallet = sequence_create_wallet_of_type(target_wallet_type);
    }
    REQUIRE(wallet, "wallet selection failed");

    waas_wallet_complete_auth_response_free(response);
    free(response);

    printf("Wallet address: %s\n", wallet->address);

    // **
    // SIGN MESSAGE
    // **

    READ_LINE("Enter message to sign: ", message);
    waas_wallet_sign_message_response *signature = sequence_sign_message("80002", message);
    REQUIRE(signature, "sign_message failed");
    printf(
        "Signature from '%s': %s\n",
        message,
        signature ? signature->signature : "(null)");

    SequenceIsValidMessageSignatureReturn *is_valid_message_signature = sequence_is_valid_message_signature(
        "80002",
        wallet->address,
        message,
        signature ? signature->signature : NULL);
    printf("Signature valid: %s (status=%d)\n",
        (is_valid_message_signature && is_valid_message_signature->is_valid) ? "true" : "false",
        is_valid_message_signature ? is_valid_message_signature->status : -1);
    free_sequence_is_valid_message_signature_return(is_valid_message_signature);
    if (signature) {
        waas_wallet_sign_message_response_free(signature);
        free(signature);
    }

    // **
    // SEND TRANSACTION
    // **

    const char *to = "0xE5E8B483FfC05967FcFed58cc98D053265af6D99";
    const char *value = "0";

    waas_wallet_send_transaction_response *tx = sequence_send_transaction("80002", to, value);
    REQUIRE(tx, "send_transaction failed");
    printf(
        "Transaction hash: %s\n",
        (tx && tx->response) ? tx->response->tx_hash : "(null)");
    if (tx) {
        waas_wallet_send_transaction_response_free(tx);
        free(tx);
    }
    waas_wallet_free(wallet);
    free(wallet);

    return 0;
}
