#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "wallet/oms_wallet_config.h"
#include "indexer/get_token_balances.h"
#include "infrastructure/is_valid_message_signature.h"
#include "storage/secure_storage.h"
#include "wallet/oms_wallet.h"

static const char *find_arg_value(int argc, char **argv, const char *name)
{
    for (int i = 2; i + 1 < argc; ++i) {
        if (strcmp(argv[i], name) == 0) {
            return argv[i + 1];
        }
    }
    return NULL;
}

static const char *find_arg_value2(
    int argc,
    char **argv,
    const char *first_name,
    const char *second_name
)
{
    const char *value = find_arg_value(argc, argv, first_name);
    return value ? value : find_arg_value(argc, argv, second_name);
}

static bool has_arg(int argc, char **argv, const char *name)
{
    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], name) == 0) {
            return true;
        }
    }
    return false;
}

static void init_oms_wallet_config_from_storage(void)
{
    char *access_key = NULL;

    secure_store_read_string("access-key", &access_key);
    if (oms_wallet_config_init(access_key) != 0) {
        fprintf(stderr, "Failed to initialize OMS Wallet config\n");
    }
    free(access_key);
}

static void free_wallet_response(oms_wallet_t *wallet)
{
    oms_wallet_free(wallet);
}

static void free_complete_auth_response(oms_wallet_complete_auth_response_t *response)
{
    oms_wallet_free_complete_auth(response);
}

static void free_sign_message_response(oms_wallet_sign_message_response_t *response)
{
    oms_wallet_free_sign_message(response);
}

static void free_send_transaction_response(oms_wallet_send_transaction_response_t *response)
{
    oms_wallet_free_send_transaction(response);
}

static void print_header(const char *title) {
    printf("\n##### %s #####\n\n", title);
}

static void print_use_case(const char *title, const char *command) {
    printf("\n");
    printf("%s\n", title);
    printf(">> %s\n", command);
}

static int require_restored_session(const char *operation, const char *missing_message)
{
    int restore_status = oms_wallet_restore_session();

    if (restore_status > 0) {
        return 1;
    }

    if (restore_status == 0) {
        fprintf(stderr, "%s\n", missing_message);
    } else {
        fprintf(stderr, "Failed to restore session for %s\n", operation);
    }

    return 0;
}

static void print_first_steps(void) {
    printf("\nLet's get things rolling!\n");
    print_use_case("Get Token Balances", "oms-wallet get-token-balances --chain-id <chain-id> --contract-address <address> --wallet-address <address> --include-metadata");
    print_use_case("Sign In with Email", "oms-wallet sign-in-with-email --email <email>");
    print_use_case("Verify Signature", "oms-wallet verify-signature --chain-id <chain-id> --wallet-address <address> --message <message> --signature <signature>");
}

static void print_use_cases(void) {
    printf("\nLet's try out some features!\n");
    print_use_case("Sign Message", "oms-wallet sign-message --chain-id <chain-id> --message <message>");
    print_use_case("Send Transaction", "oms-wallet send-transaction --chain-id <chain-id> --to <address> --value <value>");
    print_use_case("Verify Signature", "oms-wallet verify-signature --chain-id <chain-id> --wallet-address <address> --message <message> --signature <signature>");
}

static void print_wallet_and_use_cases(const oms_wallet_t *wallet)
{
    printf("Wallet ID: %s\n", wallet->id);
    printf("Wallet Address: %s\n", wallet->address);
    print_use_cases();
}

static oms_wallet_t *select_wallet_from_auth(
    const oms_wallet_complete_auth_response_t *response,
    const char *wallet_type
)
{
    oms_wallet_t *wallet = NULL;

    if (!response || !wallet_type) {
        return NULL;
    }

    if (!response->complete_auth_response || response->complete_auth_response->wallets.count == 0) {
        return oms_wallet_create_wallet_of_type(wallet_type);
    }

    if (response->complete_auth_response->wallets.items) {
        for (size_t i = 0; i < response->complete_auth_response->wallets.count; ++i) {
            if (response->complete_auth_response->wallets.items[i] &&
                strcmp(
                    waas_wallet_type_to_string(response->complete_auth_response->wallets.items[i]->type),
                    wallet_type) == 0) {
                wallet = oms_wallet_use_wallet(response->complete_auth_response->wallets.items[i]->id);
                break;
            }
        }
    }

    return wallet ? wallet : oms_wallet_create_wallet_of_type(wallet_type);
}

static void print_help(const char *prog) {
    printf(
        "OMS Wallet CLI\n"
       "\n"
       "Usage:\n"
       "  %s <command> [options]\n"
       "\n"
       "Commands:\n"
       "  init\n"
       "      Initialize project\n"
       "      --access-key <access-key>\n"
       "\n"
       "  get-token-balances\n"
       "      Fetch token balances\n"
       "      --chain-id <chain-id>\n"
       "      --contract-address <address>\n"
       "      --wallet-address <address>\n"
       "      --include-metadata\n"
       "\n"
       "  sign-in-with-email\n"
       "      Start email sign-in\n"
       "      --email <email>\n"
       "\n"
        "  confirm-email-sign-in\n"
        "      Confirm email sign-in\n"
        "      --code <code>\n"
        "      --wallet-type <ethereum> (optional)\n"
        "\n"
        "  use-wallet\n"
        "      Select wallet by id\n"
        "      --wallet-id <wallet-id>\n"
       "\n"
       "  create-wallet\n"
       "      Create a new wallet\n"
       "      --wallet-type <wallet-type> (optional)\n"
       "\n"
        "  sign-message\n"
        "      Sign a message\n"
        "      --chain-id <chain-id>\n"
        "      --message <message>\n"
        "\n"
        "  verify-signature\n"
        "      Verify a message signature\n"
        "      --chain-id <chain-id>\n"
        "      --wallet-address <address>\n"
        "      --message <message>\n"
        "      --signature <signature>\n"
        "\n"
        "  send-transaction\n"
        "      Send a transaction\n"
        "      --chain-id <chain-id>\n"
        "      --to <address>\n"
        "      --value <value-wei>\n"
       "\n"
       "Options:\n"
       "  -h, --help     Show this help message\n",
        prog
    );
}

int main(int argc, char **argv) {
    const char *cmd = NULL;

    if (argc < 2) {
        printf("Usage: oms-wallet <command> [options]\n");
        return 1;
    }

    cmd = argv[1];

    if (strcmp(cmd, "init") != 0 && strcmp(argv[1], "--help") != 0 && strcmp(argv[1], "-h") != 0) {
        init_oms_wallet_config_from_storage();
    }

    if (strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0) {

        print_help("oms-wallet");

    } else if (strcmp(cmd, "init") == 0) {
        print_header("Initialization");

        const char *access_key = NULL;

        access_key = find_arg_value(argc, argv, "--access-key");

        if (!access_key) {
            fprintf(stderr, "Missing --access-key\n");
            return 1;
        }

        secure_store_write_string("access-key", access_key);

        printf("The access key has been successfully initialized.\n");
        print_first_steps();

    } else if (strcmp(cmd, "get-token-balances") == 0) {
        print_header("Get Token Balances");

        const char *chain_id = NULL;
        const char *contract_address = NULL;
        const char *wallet_address = NULL;
        bool include_metadata = false;

        chain_id = find_arg_value(argc, argv, "--chain-id");
        contract_address = find_arg_value(argc, argv, "--contract-address");
        wallet_address = find_arg_value(argc, argv, "--wallet-address");
        include_metadata = has_arg(argc, argv, "--include-metadata");

        if (!chain_id || !contract_address || !wallet_address) {
            fprintf(stderr, "Missing required args for get-token-balances\n");
            return 1;
        }

        OmsWalletGetTokenBalancesReturn *res =
            oms_wallet_get_token_balances(chain_id, contract_address, wallet_address, include_metadata);

        if (!res) {
            fprintf(stderr, "oms_wallet_get_token_balances failed\n");
            return 1;
        }

        log_oms_wallet_get_token_balances_return(res);
        oms_wallet_free_token_balances_return(res);

    } else if (strcmp(cmd, "sign-in-with-email") == 0) {
        print_header("Sign In with Email");

        const char *email = find_arg_value(argc, argv, "--email");
        if (!email) {
            fprintf(stderr, "Missing --email\n");
            return 1;
        }

        if (oms_wallet_start_email_sign_in(email)) {
            printf("Email sign-in has been successfully initialized. Please use the code sent to your email with the following command:\n");
            print_use_case("Confirm Email Sign In", "oms-wallet confirm-email-sign-in --code <code>");
        }
    } else if (strcmp(cmd, "confirm-email-sign-in") == 0) {
        print_header("Confirming Email Sign In");

        const char *code = find_arg_value(argc, argv, "--code");
        const char *wallet_type = find_arg_value(argc, argv, "--wallet-type");
        if (!code) {
            fprintf(stderr, "Missing --code\n");
            return 1;
        }

        if (!require_restored_session(
                "confirm-email-sign-in",
                "No pending sign-in session found. Start with 'oms-wallet sign-in-with-email --email <email>'.")) {
            return 1;
        }
        oms_wallet_complete_auth_response_t *res = oms_wallet_complete_email_sign_in(code);

        if (!res) {
            fprintf(stderr, "oms_wallet_complete_email_sign_in failed\n");
            return 1;
        }

        if (wallet_type) {
            oms_wallet_t *wallet = select_wallet_from_auth(res, wallet_type);

            if (wallet) {
                print_wallet_and_use_cases(wallet);
                free_wallet_response(wallet);
            } else {
                fprintf(stderr, "Failed to initialize wallet\n");
            }

        } else {

            if (!res->complete_auth_response || res->complete_auth_response->wallets.count == 0) {
                printf("No wallets available, please create a new wallet.\n");
                print_use_case("Create Wallet", "oms-wallet create-wallet");

                free_complete_auth_response(res);
                return 0;
            }

            printf("Available wallets:\n");

            if (res->complete_auth_response && res->complete_auth_response->wallets.items) {
                for (size_t i = 0; i < res->complete_auth_response->wallets.count; ++i) {
                    if (res->complete_auth_response->wallets.items[i]) {
                        printf(
                            "Wallet %zu: id=%s type=%s address=%s\n",
                            i,
                            res->complete_auth_response->wallets.items[i]->id,
                            waas_wallet_type_to_string(res->complete_auth_response->wallets.items[i]->type),
                            res->complete_auth_response->wallets.items[i]->address);
                    }
                }
            }

            printf("\nUse the following command to select it:\n");
            print_use_case(
                "Use Wallet",
                "oms-wallet use-wallet --wallet-id <Wallet ID>"
            );
        }

        free_complete_auth_response(res);

    } else if (strcmp(cmd, "create-wallet") == 0) {
        print_header("Create Wallet");

        if (!require_restored_session(
                "create-wallet",
                "No saved session found. Complete sign-in first.")) {
            return 1;
        }

        const char *wallet_type = find_arg_value(argc, argv, "--wallet-type");

        if (!wallet_type) {
            wallet_type = "ethereum";
        }

        oms_wallet_t *wallet = wallet_type
            ? oms_wallet_create_wallet_of_type(wallet_type)
            : oms_wallet_create_wallet();

        if (!wallet) {
            fprintf(stderr, "Failed to create wallet of type '%s'\n", wallet_type);
            return 1;
        }

        print_wallet_and_use_cases(wallet);
        free_wallet_response(wallet);

    } else if (strcmp(cmd, "use-wallet") == 0) {
        print_header("Use Wallet");

        const char *wallet_id = find_arg_value(argc, argv, "--wallet-id");
        if (!wallet_id) { fprintf(stderr, "Missing --wallet-id\n"); return 1; }

        if (!require_restored_session(
                "use-wallet",
                "No saved session found. Complete sign-in first.")) {
            return 1;
        }
        oms_wallet_t *wallet = oms_wallet_use_wallet(wallet_id);

        if (!wallet) {
            fprintf(stderr, "Failed to use wallet '%s'\n", wallet_id);
            return 1;
        }

        print_wallet_and_use_cases(wallet);
        free_wallet_response(wallet);

    } else if (strcmp(cmd, "verify-signature") == 0 || strcmp(cmd, "verify_signature") == 0) {
        print_header("Verify Signature");

        const char *chain_id = NULL;
        const char *wallet_address = NULL;
        const char *message = NULL;
        const char *signature = NULL;
        chain_id = find_arg_value2(argc, argv, "--chain-id", "--chain_id");
        wallet_address = find_arg_value2(argc, argv, "--wallet-address", "--wallet_address");
        message = find_arg_value(argc, argv, "--message");
        signature = find_arg_value(argc, argv, "--signature");
        if (!chain_id || !wallet_address || !message || !signature) {
            fprintf(stderr, "Missing --chain-id, --wallet-address, --message or --signature\n");
            return 1;
        }

        OmsWalletIsValidMessageSignatureReturn *res =
            oms_wallet_is_valid_message_signature(chain_id, wallet_address, message, signature);

        printf("Status: %d\n", res ? res->status : -1);
        printf("isValid: %s\n", (res && res->is_valid) ? "true" : "false");
        oms_wallet_free_is_valid_message_signature_return(res);

    } else if (strcmp(cmd, "sign-message") == 0) {
        print_header("Sign Message");

        const char *chain_id = NULL;
        const char *message = NULL;
        chain_id = find_arg_value(argc, argv, "--chain-id");
        message = find_arg_value(argc, argv, "--message");

        if (!chain_id || !message) {
            fprintf(stderr, "Missing --chain-id or --message\n");
            return 1;
        }

        if (!require_restored_session(
                "sign-message",
                "No saved session found. Select or create a wallet first.")) {
            return 1;
        }
        oms_wallet_sign_message_response_t *signature = oms_wallet_sign_message(chain_id, message);
        printf(
            "Signature: %s\n",
            (signature && signature->sign_message_response) ? signature->sign_message_response->signature : "(null)");
        free_sign_message_response(signature);

    } else if (strcmp(cmd, "send-transaction") == 0) {
        print_header("Send Transaction");

        const char *chain_id = NULL;
        const char *to = NULL;
        const char *value = NULL;
        chain_id = find_arg_value(argc, argv, "--chain-id");
        to = find_arg_value(argc, argv, "--to");
        value = find_arg_value(argc, argv, "--value");
        if (!chain_id || !to || !value) {
            fprintf(stderr, "Missing --chain-id, --to or --value\n");
            return 1;
        }

        if (!require_restored_session(
                "send-transaction",
                "No saved session found. Select or create a wallet first.")) {
            return 1;
        }
        oms_wallet_send_transaction_response_t *tx = oms_wallet_send_transaction(chain_id, to, value);
        printf(
            "Transaction Hash: %s\n",
            (tx && tx->send_transaction_response) ? tx->send_transaction_response->tx_hash : "(null)");
        free_send_transaction_response(tx);

    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        return 1;
    }

    return 0;
}
