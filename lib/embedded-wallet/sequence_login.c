#include "sequence_login.h"

#include <ctype.h>
#include <secp256k1_recovery.h>
#include <stdio.h>
#include <string.h>

#include "evm/eoa_wallet.h"
#include "utils/globals.h"
#include "evm/keccak256.h"
#include "evm/sign_message.h"
#include "networking/http_client.h"
#include "requests/build_commit_verifier_json.h"
#include "requests/build_complete_auth_json.h"
#include "requests/initiate_auth_intent_return.h"
#include "requests/open_session_intent_return.h"
#include "utils/hex_utils.h"
#include "utils/string_utils.h"

static eoa_wallet_t *cur_signer = NULL;
static char *cur_challenge = NULL;

static char *sign_and_send(const char *endpoint, const char *payload)
{
    char *seckeyHex = bytes_to_hex(cur_signer->seckey, 32);

    printf("seckeyHex: %s", seckeyHex);

    const char *address = eoa_wallet_get_address(cur_signer->ctx, &cur_signer->pubkey);

    const char *tmpl = "POST /rpc/Wallet{0}\n\n{1}";
    const char *args[] = { endpoint, payload };
    const char *data_to_sign = format_placeholders(tmpl, args, 2);

    printf("%s\n",data_to_sign);

    uint8_t hashed_to_sign[32];
    keccak256((const uint8_t*)data_to_sign, strlen(data_to_sign), hashed_to_sign);

    char *digest32Hex = bytes_to_hex(hashed_to_sign, 32);

    printf("digest32Hex: %s\n", digest32Hex);

    char *sig = wallet_sign_message_hex_eip191(cur_signer->ctx, cur_signer->seckey, data_to_sign);

    HttpClient *c = http_client_create(g_wallet_api_url);
    if (!c) {
        fprintf(stderr, "Failed to create HttpClient\n");
        return NULL;
    }

    const char *header_template = "Authorization: Ethereum_Secp256k1 scope=\"{0}\",cred=\"{1}\",sig=\"{2}\"";
    const char *header_args[] = { "@1:test", address, sig };
    const char *auth_header = format_placeholders(header_template, header_args, 3);

    http_add_sequence_access_key(c);
    http_client_add_header(c, "Origin: http://localhost:3000");
    http_client_add_header(c, "Accept: application/json");
    http_client_add_header(c, auth_header);

    printf(">> %s (with header: %s)\n", payload, auth_header);

    HttpResponse r = http_client_post_json(c, endpoint, payload, 10000);

    if (r.error) {
        fprintf(stderr, "Request failed: %s\n", r.error);
        free(cur_signer);
        cur_signer = NULL;
        http_response_free(&r);
        http_client_destroy(c);
        return NULL;
    }

    char *body = r.body;

    printf("Response: %s\n", body);

    //http_response_free(&r);
    http_client_destroy(c);

    return body;
}

int sign_in_with_email(const char *email) {
    cur_signer = calloc(1, sizeof(*cur_signer)); // sizeof(eoa_wallet_t)
    if (!cur_signer) return -1;

    if (eoa_wallet_initialize(cur_signer) == 0) {
        free(cur_signer);
        cur_signer = NULL;
        return -1;
    }

    char *commit_verifier_json = sequence_build_commit_verifier_json(email);

    const char *body = sign_and_send("/CommitVerifier", commit_verifier_json);

    SequenceInitiateAuthResponse response = sequence_build_initiate_auth_intent_return(body);
    cur_challenge = strdup(response.data.challenge);

    return 1;
}

sequence_wallet_t *confirm_email_sign_in(const char *email, const char *code) {
    if (!cur_signer || !cur_signer->ctx) {
        fprintf(stderr, "No signer initialized\n");
        return NULL;
    }

    if (!cur_challenge) {
        fprintf(stderr, "No challenge available\n");
        free(cur_signer);
        cur_signer = NULL;
        return NULL;
    }

    char *complete_auth_json = sequence_build_complete_auth_json("", "");

    const char *body = sign_and_send("/CompleteAuth", complete_auth_json);

    SequenceSessionOpenedResult sessionData = sequence_build_open_session_intent_return(body);

    const char *new_session_id = sessionData.responseData.sessionId;
    const char *wallet_address = sessionData.responseData.wallet;

    sequence_wallet_t *sequence_wallet = sequence_wallet_from_response(
        wallet_address,
        email,
        new_session_id,
        cur_signer->seckey);

    return sequence_wallet;
}
