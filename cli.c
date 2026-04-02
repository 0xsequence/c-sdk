#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "wallet/sequence_config.h"
#include "wallet/sequence_wallet.h"
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
    print_use_case("Get Token Balances", "sequence-cli get_token_balances --chain_id <chain_id> --contract_address <address> --wallet_address <address> --include_metadata");
    print_use_case("Sign In with Email", "sequence-cli sign_in_with_email --email <email>");
    print_use_case("Verify Signature", "sequence-cli verify_signature --chain_id <chain_id> --wallet_address <address> --message <message> --signature <signature>");
}

void print_use_cases() {
    printf("\nLet’s try out some features!\n");
    print_use_case("Sign Message", "sequence-cli sign_message --chain_id <chain_id> --message <message>");
    print_use_case("Send Transaction", "sequence-cli send_transaction --chain_id <chain_id> --to <address> --value <value>");
    print_use_case("Verify Signature", "sequence-cli verify_signature --chain_id <chain_id> --wallet_address <address> --message <message> --signature <signature>");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: sequence <command> [options]\n");
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "init") != 0) {
        char *access_key = NULL;
        secure_store_read_string("access_key", &access_key);
        sequence_config_init(access_key);
    }

    if (strcmp(cmd, "init") == 0) {
        print_header("Initialization");

        const char *access_key = NULL;

        // Parse CLI args
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--access_key") == 0 && i + 1 < argc) {
                access_key = argv[++i];
            }
        }

        if (!access_key) {
            fprintf(stderr, "Missing --access_key\n");
            return 1;
        }

        secure_store_write_string("access_key", access_key);

        printf("The access key has been successfully initialized.\n");
        print_first_steps();

    } else if (strcmp(cmd, "get_token_balances") == 0) {
        print_header("Get Token Balances");

        const char *chain_id = NULL;
        const char *contract_address = NULL;
        const char *wallet_address = NULL;
        bool include_metadata = false;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--chain_id") == 0 && i + 1 < argc) chain_id = argv[++i];
            else if (strcmp(argv[i], "--contract_address") == 0 && i + 1 < argc) contract_address = argv[++i];
            else if (strcmp(argv[i], "--wallet_address") == 0 && i + 1 < argc) wallet_address = argv[++i];
            else if (strcmp(argv[i], "--include_metadata") == 0) include_metadata = true;
        }

        if (!chain_id || !contract_address || !wallet_address) {
            fprintf(stderr, "Missing required args for get_token_balances\n");
            return 1;
        }

        SequenceGetTokenBalancesReturn *res =
            sequence_get_token_balances(chain_id, contract_address, wallet_address, include_metadata);

        log_sequence_get_token_balances_return(res);
        free_sequence_token_balances_return(res);

    } else if (strcmp(cmd, "sign_in_with_email") == 0) {
        print_header("Sign In with Email");

        const char *email = NULL;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--email") == 0 && i + 1 < argc) email = argv[++i];
        }
        if (!email) { fprintf(stderr, "Missing --email\n"); return 1; }

        if (sequence_sign_in_with_email(email)) {
            printf("Email sign-in has been successfully initialized. Please use the code sent to your email with the following command:\n");
            print_use_case("Confirm Email Sign In", "sequence-cli confirm_email_sign_in --email <email> --code <code>");
        }
    } else if (strcmp(cmd, "confirm_email_sign_in") == 0) {
        print_header("Confirming Email Sign In");

        const char *email = NULL;
        const char *code = NULL;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--email") == 0 && i + 1 < argc) email = argv[++i];
            else if (strcmp(argv[i], "--code") == 0 && i + 1 < argc) code = argv[++i];
        }
        if (!email || !code) { fprintf(stderr, "Missing --email or --code\n"); return 1; }

        sequence_restore_session();
        sequence_complete_auth_return *res = sequence_confirm_email_sign_in(email, code);

        if (res->wallet_count <= 0) {
            printf("No wallets available, please create a new wallet.");
            print_use_case("Create Wallet", "sequence-cli create_wallet");
        } else {
            printf("Please select one of the existing wallet types:\n");
        }

        if (res->wallets) {
            for (size_t i = 0; i < res->wallet_count; i++) {
                printf("Wallet Type %ld: %s\n", i, res->wallets[i].type);
            }
        }

        sequence_complete_auth_return_free(res);

        printf("\nUse the following command to select it:\n");
        print_use_case("Use Wallet", "sequence-cli use_wallet --wallet_type <Wallet Type>");

    } else if (strcmp(cmd, "create_wallet") == 0) {
        print_header("Create Wallet");

        sequence_restore_session();
        const sequence_wallet *wallet = sequence_create_wallet();
        printf("Sequence Wallet Address: %s\n", wallet->address);

        print_use_cases();

    } else if (strcmp(cmd, "use_wallet") == 0) {
        print_header("Use Wallet");

        const char *walletType = NULL;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--wallet_type") == 0 && i + 1 < argc) walletType = argv[++i];
        }
        if (!walletType) { fprintf(stderr, "Missing --wallet_type\n"); return 1; }

        sequence_restore_session();
        const sequence_wallet *wallet = sequence_use_wallet(walletType);
        printf("Sequence Wallet Address: %s\n", wallet->address);

        print_use_cases();

    } else if (strcmp(cmd, "verify_signature") == 0) {
        print_header("Verify Signature");

        const char *chain_id = NULL;
        const char *wallet_address = NULL;
        const char *message = NULL;
        const char *signature = NULL;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--chain_id") == 0 && i + 1 < argc) chain_id = argv[++i];
            else if (strcmp(argv[i], "--wallet_address") == 0 && i + 1 < argc) wallet_address = argv[++i];
            else if (strcmp(argv[i], "--message") == 0 && i + 1 < argc) message = argv[++i];
            else if (strcmp(argv[i], "--signature") == 0 && i + 1 < argc) signature = argv[++i];
        }
        if (!chain_id || !wallet_address || !message || !signature) {
            fprintf(stderr, "Missing --chain_id, --wallet_address, --message or --signature\n");
            return 1;
        }

        SequenceIsValidMessageSignatureReturn *res =
            sequence_is_valid_message_signature(chain_id, wallet_address, message, signature);

        printf("Status: %d\n", res ? res->status : -1);
        printf("isValid: %s\n", (res && res->is_valid) ? "true" : "false");
        free_sequence_is_valid_message_signature_return(res);

    } else if (strcmp(cmd, "sign_message") == 0) {
        print_header("Sign Message");

        const char *chain_id = NULL;
        const char *message = NULL;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--chain_id") == 0 && i + 1 < argc) chain_id = argv[++i];
            else if (strcmp(argv[i], "--message") == 0 && i + 1 < argc) message = argv[++i];
        }
        if (!chain_id || !message) { fprintf(stderr, "Missing --chain_id or --message\n"); return 1; }

        sequence_restore_session();
        char *signature = sequence_sign_message(chain_id, message);
        printf("Signature: %s\n", signature);

    } else if (strcmp(cmd, "send_transaction") == 0) {
        print_header("Send Transaction");

        const char *chain_id = NULL;
        const char *to = NULL;
        const char *value = NULL;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--chain_id") == 0 && i + 1 < argc) chain_id = argv[++i];
            else if (strcmp(argv[i], "--to") == 0 && i + 1 < argc) to = argv[++i];
            else if (strcmp(argv[i], "--value") == 0 && i + 1 < argc) value = argv[++i];
        }
        if (!chain_id || !to || !value) { fprintf(stderr, "Missing --chain_id, --to or --value\n"); return 1; }

        sequence_restore_session();
        char *txHash = sequence_send_transaction(chain_id, to, value);
        printf("Transaction Hash: %s\n", txHash);

    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        return 1;
    }

    return 0;
}
