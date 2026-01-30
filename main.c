#include <stdio.h>
#include <string.h>
#include "lib/indexer/get_token_balances.h"
#include "lib/embedded-wallet/sequence_config.h"
#include "lib/embedded-wallet/sequence_login.h"
#include "lib/embedded-wallet/sequence_wallet.h"
#include "lib/evm/sign_message.h"

static void strip_newline(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[n - 1] = '\0';
        n--;
    }
}

static void hexprint(const char *label, const unsigned char *buf, size_t len) {
    printf("%s", label);
    for (size_t i = 0; i < len; i++) printf("%02x", buf[i]);
    printf("\n");
}

int main(void) {
    sequence_config_init("oesk7yu5tjNfQElu5HjuUunAAAAAAAAAA");

    SequenceGetTokenBalancesReturn *tokenBalances = sequence_get_token_balances(137, "0x3c499c542cEF5E3811e1192ce70d8cC03d5c3359", "0x8e3E38fe7367dd3b52D1e281E4e8400447C8d8B9", true);
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
    if (!wallet) {
        fprintf(stderr, "confirm_email_sign_in failed\n");
        return 1;
    }

    // Log public key (uncompressed 65 bytes)
    unsigned char pub65[65];
    size_t pub_len = sequence_wallet_serialize_pubkey(wallet, pub65, 0);
    if (pub_len == 0) {
        fprintf(stderr, "Failed to serialize pubkey\n");
        sequence_wallet_clear(wallet);
        return 1;
    }

    /*char *sig = wallet_sign_string_hex_eip191(w.seckey, ctx, "0x1234", NULL, 0);
    printf("sig = %s\n", sig);*/

    hexprint("Public key (65, uncompressed): 0x", pub65, pub_len);

    // If you also want to log the private key (careful!):
    // hexprint("Private key (32): 0x", wallet->seckey, 32);

    sequence_wallet_clear(wallet);
    return 0;
}