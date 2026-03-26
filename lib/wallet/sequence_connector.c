#include "sequence_connector.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "chains/chain_bindings.h"
#include "evm/eoa_wallet.h"
#include "utils/globals.h"
#include "evm/keccak256.h"
#include "evm/sign_message.h"
#include "networking/http_client.h"
#include "requests/build_commit_verifier_json.h"
#include "requests/build_complete_auth_json.h"
#include "requests/build_create_wallet_json.h"
#include "requests/build_send_transaction_json.h"
#include "requests/build_sign_message_json.h"
#include "requests/build_use_wallet_json.h"
#include "requests/commit_verifier_return.h"
#include "requests/send_transaction_return.h"
#include "requests/sign_message_return.h"
#include "requests/wallet_return.h"
#include "storage/secure_storage.h"
#include "utils/hex_utils.h"
#include "utils/string_utils.h"
#include "utils/timestamps.h"

static eoa_wallet_t* cur_signer = NULL;
static char* cur_challenge = NULL;

static char* sign_and_send(const char* endpoint, const char* payload)
{
    const char* address = eoa_wallet_get_address(cur_signer->ctx, &cur_signer->pubkey);

    long nonce_int = timestamp_now_seconds();

    char nonce[32];
    snprintf(nonce, sizeof(nonce), "%ld", nonce_int);

    const char* tmpl = "POST /rpc/Wallet{0}\nnonce: {1}\n\n{2}";
    const char* args[] = {endpoint, nonce, payload};
    const char* data_to_sign = format_placeholders(tmpl, args, 3);

    uint8_t hashed_to_sign[32];
    keccak256((const uint8_t*)data_to_sign, strlen(data_to_sign), hashed_to_sign);

    char* sig = wallet_sign_message_hex_eip191(cur_signer->ctx, cur_signer->seckey, data_to_sign);

    HttpClient* c = http_client_create(g_wallet_api_url);
    if (!c)
    {
        fprintf(stderr, "Failed to create HttpClient\n");
        return NULL;
    }

    const char* header_template =
        "Authorization: Ethereum_Secp256k1 scope=\"{0}\",cred=\"{1}\",nonce={2},sig=\"{3}\"";
    const char* header_args[] = {"@1:test", address, nonce, sig};
    const char* auth_header = format_placeholders(header_template, header_args, 4);

    http_add_sequence_access_key(c);
    http_client_add_header(c, "Origin: http://localhost:3000");
    http_client_add_header(c, "Accept: application/json");
    http_client_add_header(c, auth_header);

    printf(">> Request\n%s\n%s\n\n", payload, auth_header);

    HttpResponse r = http_client_post_json(c, endpoint, payload, 10000);

    if (r.error)
    {
        fprintf(stderr, "Request failed: %s\n", r.error);
        free(cur_signer);
        cur_signer = NULL;
        http_response_free(&r);
        http_client_destroy(c);
        return NULL;
    }

    char* body = strdup(r.body);

    printf("<< Response\n%s\n\n", body);

    http_response_free(&r);
    http_client_destroy(c);

    return body;
}

int sequence_restore_session()
{
    uint8_t seckey[32];
    int status = secure_store_read_seckey(seckey);

    if (status != 0)
    {
        printf("Failed to read seckey (error=%d)\n", status);
        return -1;
    }

    cur_signer = calloc(1, sizeof(*cur_signer));
    eoa_wallet_from_private_key_bytes(cur_signer, seckey);

    cur_challenge = NULL;
    secure_store_read_string("challenge", &cur_challenge);
    return 1;
}

int sequence_sign_in_with_email(const char* email)
{
    cur_signer = calloc(1, sizeof(*cur_signer)); // sizeof(eoa_wallet_t)
    if (!cur_signer)
    {
        return -1;
    }

    if (eoa_wallet_initialize(cur_signer) == 0)
    {
        free(cur_signer);
        cur_signer = NULL;
        return -1;
    }

    const char* commit_verifier_json = sequence_build_commit_verifier_json(email);
    const char* body = sign_and_send("/CommitVerifier", commit_verifier_json);

    sequence_commit_verifier_response* response = sequence_build_commit_verifier_return(body);
    cur_challenge = strdup(response->challenge);
    secure_store_write_string("challenge", cur_challenge);

    sequence_commit_verifier_response_free(response);

    secure_store_write_seckey(cur_signer->seckey);

    return 1;
}

sequence_complete_auth_return* sequence_confirm_email_sign_in(const char* email, const char* code)
{
    if (!cur_signer || !cur_signer->ctx)
    {
        fprintf(stderr, "No signer initialized\n");
        return NULL;
    }

    if (!cur_challenge)
    {
        fprintf(stderr, "No challenge available\n");
        free(cur_signer);
        cur_signer = NULL;
        return NULL;
    }

    const char* preHashAnswer = concat_malloc(cur_challenge, code);

    uint8_t hashed_to_sign[32];
    keccak256((const uint8_t*)preHashAnswer, strlen(preHashAnswer), hashed_to_sign);

    const char* hashedAnswerHex = bytes_to_hex(hashed_to_sign, 32);
    const char* complete_auth_json = sequence_build_complete_auth_json(email, hashedAnswerHex);
    const char* body = sign_and_send("/CompleteAuth", complete_auth_json);

    sequence_complete_auth_return* response = sequence_build_complete_auth_return(body);

    return response;
}

sequence_wallet* sequence_use_wallet(const char* walletType)
{
    if (!cur_signer || !cur_signer->ctx)
    {
        fprintf(stderr, "No signer initialized\n");
        return NULL;
    }

    const char* use_wallet_json = sequence_build_use_wallet_json(walletType);
    const char* body = sign_and_send("/UseWallet", use_wallet_json);

    sequence_wallet_response* response = sequence_build_wallet_return(body);

    sequence_wallet* sequence_wallet = sequence_wallet_from_response(
        response->wallet.address,
        cur_signer->seckey);

    secure_store_write_string("sequence_wallet_address", sequence_wallet->address);

    sequence_wallet_response_free(response);

    return sequence_wallet;
}

sequence_wallet* sequence_create_wallet()
{
    if (!cur_signer || !cur_signer->ctx)
    {
        fprintf(stderr, "No signer initialized\n");
        return NULL;
    }

    const char* create_wallet_json = sequence_build_create_wallet_json("Ethereum_SequenceV3");
    const char* body = sign_and_send("/CreateWallet", create_wallet_json);

    sequence_wallet_response* response = sequence_build_wallet_return(body);

    sequence_wallet* sequence_wallet = sequence_wallet_from_response(
        response->wallet.address,
        cur_signer->seckey);

    secure_store_write_string("sequence_wallet_address", sequence_wallet->address);

    sequence_wallet_response_free(response);

    return sequence_wallet;
}

char* sequence_sign_message(const char* chain_id, const char* message)
{
    if (!cur_signer || !cur_signer->ctx)
    {
        fprintf(stderr, "No signer initialized\n");
        return NULL;
    }

    char *address = NULL;
    secure_store_read_string("sequence_wallet_address", &address);

    const char* network = sequence_get_chain_name(chain_id);

    const char* json = build_sign_message_json(address, network, message);
    const char* body = sign_and_send("/SignMessage", json);

    sequence_sign_message_response* response = sequence_build_sign_message_return(body);
    char* signature = strdup(response->signature);

    sequence_sign_message_response_free(response);

    return signature;
}

char* sequence_send_transaction(const char* chain_id, const char* to, const char* value)
{
    if (!cur_signer || !cur_signer->ctx)
    {
        fprintf(stderr, "No signer initialized\n");
        return NULL;
    }

    char *address = NULL;
    secure_store_read_string("sequence_wallet_address", &address);

    const char* network = sequence_get_chain_name(chain_id);

    const char* json = build_send_transaction_json(address, network, to, value);
    const char* body = sign_and_send("/SendTransaction", json);

    sequence_send_transaction_response* response = sequence_build_send_transaction_return(body);
    char* txHash = strdup(response->response.txHash);

    sequence_send_transaction_response_free(response);

    return txHash;
}
