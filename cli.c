#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <xpc/xpc.h>

#include "wallet/sequence_config.h"
#include "indexer/get_token_balances.h"
#include "infrastructure/is_valid_message_signature.h"
#include "storage/secure_storage.h"
#include "wallet/sequence_connector.h"

// Helper: print string safely
void print_result(char *res) {
    if (res) {
        printf("%s\n", res);
        free(res);
    } else {
        printf("null\n");
    }
}

void print_header(char *title) {
    printf("\n##### %s #####\n\n", title);
}

void print_use_case(char *title, char *command) {
    printf("\n");
    printf("%s\n", title);
    printf(">> %s\n", command);
}

void print_first_steps() {
    printf("\nLet's get things rolling!\n");
    print_use_case("Get Token Balances", "sequence-wallet get-token-balances --chain-id <chain-id> --contract-address <address> --wallet-address <address> --include-metadata");
    print_use_case("Sign In with Email", "sequence-wallet sign-in-with-email --email <email>");
    print_use_case("Verify Signature", "sequence-wallet verify-signature --chain-id <chain-id> --wallet-address <address> --message <message> --signature <signature>");
}

void print_use_cases() {
    printf("\nLet’s try out some features!\n");
    print_use_case("Sign Message", "sequence-wallet sign-message --chain-id <chain-id> --message <message>");
    print_use_case("Send Transaction", "sequence-wallet send-transaction --chain-id <chain-id> --to <address> --value <value>");
    print_use_case("Verify Signature", "sequence-wallet verify-signature --chain-id <chain-id> --wallet-address <address> --message <message> --signature <signature>");
}

void print_help(const char *prog) {
    printf(
        "Sequence Wallet CLI\n"
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
        "      --email <email>\n"
        "      --code <code>\n"
        "      --wallet-type <Ethereum_SequenceV3 | Ethereum_EOA> (optional)\n"
        "\n"
        "  use-wallet\n"
        "      Select wallet type\n"
        "      --wallet-type <wallet-type>\n"
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
    if (argc < 2) {
        printf("Usage: sequence-wallet <command> [options]\n");
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "init") != 0 && strcmp(argv[1], "--help") != 0 && strcmp(argv[1], "-h") != 0) {
        char *access_key = NULL;
        secure_store_read_string("access-key", &access_key);
        sequence_config_init(access_key);
    }

    if (strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0) {

        print_help("sequence-wallet");

    } else if (strcmp(cmd, "init") == 0) {
        print_header("Initialization");

        const char *access_key = NULL;

        // Parse CLI args
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--access-key") == 0 && i + 1 < argc) {
                access_key = argv[++i];
            }
        }

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

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--chain-id") == 0 && i + 1 < argc) chain_id = argv[++i];
            else if (strcmp(argv[i], "--contract-address") == 0 && i + 1 < argc) contract_address = argv[++i];
            else if (strcmp(argv[i], "--wallet-address") == 0 && i + 1 < argc) wallet_address = argv[++i];
            else if (strcmp(argv[i], "--include-metadata") == 0) include_metadata = true;
        }

        if (!chain_id || !contract_address || !wallet_address) {
            fprintf(stderr, "Missing required args for get-token-balances\n");
            return 1;
        }

        SequenceGetTokenBalancesReturn *res =
            sequence_get_token_balances(chain_id, contract_address, wallet_address, include_metadata);

        if (!res) {
            fprintf(stderr, "sequence_get_token_balances failed\n");
            return 1;
        }

        log_sequence_get_token_balances_return(res);
        free_sequence_token_balances_return(res);

    } else if (strcmp(cmd, "sign-in-with-email") == 0) {
        print_header("Sign In with Email");

        const char *email = NULL;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--email") == 0 && i + 1 < argc) email = argv[++i];
        }
        if (!email) {
            fprintf(stderr, "Missing --email\n");
            return 1;
        }

        if (sequence_sign_in_with_email(email)) {
            printf("Email sign-in has been successfully initialized. Please use the code sent to your email with the following command:\n");
            print_use_case("Confirm Email Sign In", "sequence-wallet confirm-email-sign-in --email <email> --code <code>");
        }
    } else if (strcmp(cmd, "confirm-email-sign-in") == 0) {
        print_header("Confirming Email Sign In");

        const char *email = NULL;
        const char *code = NULL;
        const char *wallet_type = NULL;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--email") == 0 && i + 1 < argc) email = argv[++i];
            else if (strcmp(argv[i], "--code") == 0 && i + 1 < argc) code = argv[++i];
            else if (strcmp(argv[i], "--wallet-type") == 0) wallet_type = argv[++i];
        }
        if (!email || !code) {
            fprintf(stderr, "Missing --email or --code\n");
            return 1;
        }

        sequence_restore_session();
        waas_complete_auth_response *res = sequence_confirm_email_sign_in(email, code);

        if (!res) {
            fprintf(stderr, "sequence_confirm_email_sign_in failed\n");
            return 1;
        }

        waas_wallet *wallet = NULL;

        if (wallet_type) {
            /* No wallets → create one */
            if (res->wallets.count == 0) {
                wallet = sequence_create_wallet_of_type(wallet_type);
            }
            else if (res->wallets.items) {

                /* Prefer Ethereum_SequenceV3 if available */
                for (size_t i = 0; i < res->wallets.count; ++i) {
                    if (res->wallets.items[i] &&
                        strcmp(
                            waas_wallet_type_to_string(res->wallets.items[i]->type),
                            wallet_type) == 0) {
                        wallet = sequence_use_wallet(wallet_type);
                        break;
                    }
                }
            }

            /* If no wallet was selected, fallback to create the specified wallet type */
            if (!wallet) {
                wallet = sequence_create_wallet_of_type(wallet_type);
            }

            if (wallet) {
                printf("Sequence Wallet Address: %s\n", wallet->address);
                print_use_cases();
            } else {
                fprintf(stderr, "Failed to initialize wallet\n");
            }

        } else {

            if (res->wallets.count == 0) {
                printf("No wallets available, please create a new wallet.\n");
                print_use_case("Create Wallet", "sequence-wallet create-wallet");

                waas_complete_auth_response_free(res);
                free(res);
                return 0;
            }

            printf("Please select one of the existing wallet types:\n");

            if (res->wallets.items) {
                for (size_t i = 0; i < res->wallets.count; ++i) {
                    if (res->wallets.items[i]) {
                        printf(
                            "Wallet Type %zu: %s\n",
                            i,
                            waas_wallet_type_to_string(res->wallets.items[i]->type));
                    }
                }
            }

            printf("\nUse the following command to select it:\n");
            print_use_case(
                "Use Wallet",
                "sequence-wallet use-wallet --wallet-type <Wallet Type>"
            );
        }

        waas_complete_auth_response_free(res);
        free(res);

    } else if (strcmp(cmd, "create-wallet") == 0) {
        print_header("Create Wallet");

        sequence_restore_session();

        const char *walletType = NULL;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--wallet-type") == 0 && i + 1 < argc) walletType = argv[++i];
        }

        // Use sequence v3 wallet type by default
        if (!walletType) {
            walletType = "Ethereum_SequenceV3";
        }

        waas_wallet *wallet = walletType
            ? sequence_create_wallet_of_type(walletType)
            : sequence_create_wallet();

        if (!wallet) {
            fprintf(stderr, "Failed to create wallet of type '%s'\n", walletType);
            return 1;
        }

        printf("Sequence Wallet Address: %s\n", wallet->address);
        waas_wallet_free(wallet);
        free(wallet);

        print_use_cases();

    } else if (strcmp(cmd, "use-wallet") == 0) {
        print_header("Use Wallet");

        const char *walletType = NULL;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--wallet-type") == 0 && i + 1 < argc) walletType = argv[++i];
        }
        if (!walletType) { fprintf(stderr, "Missing --wallet-type\n"); return 1; }

        sequence_restore_session();
        waas_wallet *wallet = sequence_use_wallet(walletType);

        if (!wallet) {
            fprintf(stderr, "Failed to use wallet of type '%s'\n", walletType);
            return 1;
        }

        printf("Sequence Wallet Address: %s\n", wallet->address);
        waas_wallet_free(wallet);
        free(wallet);

        print_use_cases();

    } else if (strcmp(cmd, "verify-signature") == 0 || strcmp(cmd, "verify_signature") == 0) {
        print_header("Verify Signature");

        const char *chain_id = NULL;
        const char *wallet_address = NULL;
        const char *message = NULL;
        const char *signature = NULL;
        for (int i = 2; i < argc; i++) {
            if ((strcmp(argv[i], "--chain-id") == 0 || strcmp(argv[i], "--chain_id") == 0) && i + 1 < argc) chain_id = argv[++i];
            else if ((strcmp(argv[i], "--wallet-address") == 0 || strcmp(argv[i], "--wallet_address") == 0) && i + 1 < argc) wallet_address = argv[++i];
            else if (strcmp(argv[i], "--message") == 0 && i + 1 < argc) message = argv[++i];
            else if (strcmp(argv[i], "--signature") == 0 && i + 1 < argc) signature = argv[++i];
        }
        if (!chain_id || !wallet_address || !message || !signature) {
            fprintf(stderr, "Missing --chain-id, --wallet-address, --message or --signature\n");
            return 1;
        }

        SequenceIsValidMessageSignatureReturn *res =
            sequence_is_valid_message_signature(chain_id, wallet_address, message, signature);

        printf("Status: %d\n", res ? res->status : -1);
        printf("isValid: %s\n", (res && res->is_valid) ? "true" : "false");
        free_sequence_is_valid_message_signature_return(res);

    } else if (strcmp(cmd, "sign-message") == 0) {
        print_header("Sign Message");

        const char *chain_id = NULL;
        const char *message = NULL;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--chain-id") == 0 && i + 1 < argc) chain_id = argv[++i];
            else if (strcmp(argv[i], "--message") == 0 && i + 1 < argc) message = argv[++i];
        }

        if (!chain_id || !message) {
            fprintf(stderr, "Missing --chain-id or --message\n");
            return 1;
        }

        sequence_restore_session();
        waas_sign_message_response *signature = sequence_sign_message(chain_id, message);
        printf("Signature: %s\n", signature ? signature->signature : "(null)");
        if (signature) {
            waas_sign_message_response_free(signature);
            free(signature);
        }

    } else if (strcmp(cmd, "send-transaction") == 0) {
        print_header("Send Transaction");

        const char *chain_id = NULL;
        const char *to = NULL;
        const char *value = NULL;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--chain-id") == 0 && i + 1 < argc) chain_id = argv[++i];
            else if (strcmp(argv[i], "--to") == 0 && i + 1 < argc) to = argv[++i];
            else if (strcmp(argv[i], "--value") == 0 && i + 1 < argc) value = argv[++i];
        }
        if (!chain_id || !to || !value) {
            fprintf(stderr, "Missing --chain-id, --to or --value\n");
            return 1;
        }

        sequence_restore_session();
        waas_send_transaction_response *tx = sequence_send_transaction(chain_id, to, value);
        printf(
            "Transaction Hash: %s\n",
            (tx && tx->response) ? tx->response->tx_hash : "(null)");
        if (tx) {
            waas_send_transaction_response_free(tx);
            free(tx);
        }

    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        return 1;
    }

    return 0;
}
