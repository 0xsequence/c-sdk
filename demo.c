#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

static int wait_for_transaction_executed(
    oms_wallet_sdk_t *sdk,
    const char *txn_id,
    int max_attempts,
    unsigned int sleep_seconds)
{
    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        oms_wallet_get_transaction_status_response_t *status_response =
            oms_wallet_get_transaction_status(sdk, txn_id);
        waas_transaction_status_response *status_payload =
            status_response ? status_response->transaction_status_response : NULL;

        if (!status_payload) {
            oms_wallet_free_get_transaction_status(status_response);
            fprintf(stderr, "get_transaction_status failed\n");
            return -1;
        }

        printf(
            "Transaction status [%d/%d]: %s",
            attempt,
            max_attempts,
            waas_transaction_status_to_string(status_payload->status));
        if (status_payload->has_txn_hash && status_payload->txn_hash) {
            printf(" (hash=%s)", status_payload->txn_hash);
        }
        printf("\n");

        if (status_payload->status == WAAS_TRANSACTION_STATUS_EXECUTED) {
            oms_wallet_free_get_transaction_status(status_response);
            return 0;
        }

        oms_wallet_free_get_transaction_status(status_response);
        if (attempt < max_attempts) {
            sleep(sleep_seconds);
        }
    }

    return 1;
}

int main(void) {
    // **
    // SETUP
    // **

    char *access_key = "AQAAAAAAAAK2JvvZhWqZ51riasWBftkrVXE";
    oms_wallet_sdk_t sdk;
    memset(&sdk, 0, sizeof(sdk));
    REQUIRE(oms_wallet_sdk_init(&sdk, access_key) == 0, "oms_wallet_sdk_init failed");
    REQUIRE(
        oms_wallet_config_set_indexer_url_template(&sdk, "https://dev-{value}-indexer.sequence.app/rpc/Indexer/") == 0,
        "oms_wallet_config_set_indexer_url_template failed");
    REQUIRE(
        oms_wallet_config_set_api_rpc_url(&sdk, "https://dev-api.sequence.app/rpc/API") == 0,
        "oms_wallet_config_set_api_rpc_url failed");

    // **
    // INDEXER
    // **

    const char *chain_id = "137";
    const char *contract_address = "0x3c499c542cEF5E3811e1192ce70d8cC03d5c3359";
    const char *wallet_address = "0x8e3E38fe7367dd3b52D1e281E4e8400447C8d8B9";

    OmsWalletGetTokenBalancesReturn *tokenBalances = oms_wallet_get_token_balances(
        &sdk,
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
    const char *message = "test message 99922";

    READ_LINE("Enter email: ", email);
    REQUIRE(oms_wallet_start_email_sign_in(&sdk, email), "start_email_sign_in failed");

    READ_LINE("Enter code: ", code);
    oms_wallet_complete_auth_response_t *response = oms_wallet_complete_email_sign_in(&sdk, code);
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
            wallet = oms_wallet_use_wallet(&sdk, response->complete_auth_response->wallets.items[i]->id);
            break;
        }
    }

    if (!wallet)
    {
        wallet = oms_wallet_create_wallet_of_type(&sdk, target_wallet_type);
    }
    REQUIRE(wallet, "wallet selection failed");

    oms_wallet_free_complete_auth(response);

    printf("Wallet address: %s\n", wallet->address);

    // **
    // SIGN MESSAGE
    // **

    oms_wallet_sign_message_response_t *signature = oms_wallet_sign_message(&sdk, "80002", message);
    REQUIRE(signature, "sign_message failed");
    printf(
        "Signature from '%s': %s\n",
        message,
        (signature && signature->sign_message_response) ? signature->sign_message_response->signature : "(null)");

    OmsWalletIsValidMessageSignatureReturn *is_valid_message_signature = oms_wallet_is_valid_message_signature(
        &sdk,
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

    oms_wallet_prepare_ethereum_transaction_response_t *prepared =
        oms_wallet_prepare_ethereum_transaction(&sdk, "80002", to, value);
    REQUIRE(prepared && prepared->prepare_response, "prepare_ethereum_transaction failed");

    oms_wallet_execute_response_t *executed =
        oms_wallet_execute(&sdk, prepared->prepare_response->txn_id);
    REQUIRE(executed && executed->execute_response, "execute failed");
    printf(
        "Transaction accepted: %s (status=%s)\n",
        prepared->prepare_response->txn_id,
        waas_transaction_status_to_string(executed->execute_response->status));
    int wait_result = wait_for_transaction_executed(
        &sdk,
        prepared->prepare_response->txn_id,
        60,
        2);
    if (executed) {
        oms_wallet_free_execute(executed);
    }
    if (prepared) {
        oms_wallet_free_prepare_ethereum_transaction(prepared);
    }
    oms_wallet_free(wallet);
    oms_wallet_sdk_cleanup(&sdk);

    if (wait_result != 0) {
        fprintf(stderr, "transaction did not reach executed status before timeout\n");
        return 1;
    }

    return 0;
}
