#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "wallet/sequence_config.h"
#include "wallet/sequence_wallet.h"
#include "indexer/get_token_balances.h"
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

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: sequence <command> [options]\n");
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "init") != 0) {
        char *access_key = NULL;
        secure_store_read_access_key(&access_key);
        sequence_config_init(access_key);
    }

    if (strcmp(cmd, "init") == 0) {
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

        secure_store_write_access_key(access_key);
    } else if (strcmp(cmd, "get_token_balances") == 0) {
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
        const char *email = NULL;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--email") == 0 && i + 1 < argc) email = argv[++i];
        }
        if (!email) { fprintf(stderr, "Missing --email\n"); return 1; }

        int status = sequence_sign_in_with_email(email);
        printf("sign_in_with_email status: %d\n", status);

    } else if (strcmp(cmd, "confirm_email_sign_in") == 0) {
        const char *email = NULL;
        const char *code = NULL;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--email") == 0 && i + 1 < argc) email = argv[++i];
            else if (strcmp(argv[i], "--code") == 0 && i + 1 < argc) code = argv[++i];
        }
        if (!email || !code) { fprintf(stderr, "Missing --email or --code\n"); return 1; }

        sequence_restore_session();
        sequence_complete_auth_return *res = sequence_confirm_email_sign_in(email, code);
        sequence_complete_auth_return_free(res);

    } else if (strcmp(cmd, "create_wallet") == 0) {
        sequence_restore_session();
        sequence_wallet *wallet = sequence_create_wallet();
        print_result(wallet->address);
        sequence_wallet_free(wallet);

    } else if (strcmp(cmd, "use_wallet") == 0) {
        const char *walletType = NULL;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--wallet_type") == 0 && i + 1 < argc) walletType = argv[++i];
        }
        if (!walletType) { fprintf(stderr, "Missing --wallet_type\n"); return 1; }

        sequence_restore_session();
        sequence_wallet *wallet = sequence_use_wallet(walletType);
        print_result(wallet->address);
        sequence_wallet_free(wallet);

    } else if (strcmp(cmd, "sign_message") == 0) {
        const char *chain_id = NULL;
        const char *message = NULL;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--chain_id") == 0 && i + 1 < argc) chain_id = argv[++i];
            else if (strcmp(argv[i], "--message") == 0 && i + 1 < argc) message = argv[++i];
        }
        if (!chain_id || !message) { fprintf(stderr, "Missing --chain_id or --message\n"); return 1; }

        sequence_restore_session();
        char *res = sequence_sign_message(chain_id, message);
        print_result(res);

    } else if (strcmp(cmd, "send_transaction") == 0) {
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
        char *res = sequence_send_transaction(chain_id, to, value);
        print_result(res);

    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        return 1;
    }

    return 0;
}