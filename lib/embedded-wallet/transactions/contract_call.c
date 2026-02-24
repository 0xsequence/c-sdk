#include "contract_call.h"
#include "embedded-wallet/requests/transaction_intent_return.h"
#include "embedded-wallet/requests/build_contract_call_intent_json.h"
#include "embedded-wallet/requests/build_signable_intent_json.h"
#include "embedded-wallet/requests/build_intent_json.h"
#include "utils/globals.h"
#include "utils/timestamps.h"
#include "evm/keccak256.h"
#include "evm/eoa_wallet.h"
#include "evm/sign_message.h"
#include "networking/http_client.h"
#include "utils/uuid.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <inttypes.h>

char *sequence_contract_call(
    sequence_wallet *wallet,
    uint64_t chain_id,
    SequenceContractCallData *transactions,
    size_t transactions_len
) {
    HttpClient *c = http_client_create(g_waas_api_url);
    if (!c) {
        fprintf(stderr, "Failed to create HttpClient\n");
    }

    http_add_sequence_access_key(c);
    http_client_add_header(c, "Accept: application/json");

    srand((unsigned)time(NULL));

    char identifier[37];
    generate_uuid_v4(identifier);

    // Chain id

    char chain_id_str[32];
    snprintf(chain_id_str, sizeof(chain_id_str), "%" PRIu64, chain_id);

    cJSON *intent_data = sequence_build_contract_call_intent_json(
        identifier,
        chain_id_str,
        wallet->address,
        "contract_address",
        "contractCall",
        "0",
        "function_signature",
        "args[0].v.str",
        "args[1].v.str");

    long issuedAt = timestamp_now_seconds();
    long expiresAt = timestamp_seconds_from_now(36000);

    char *to_sign = build_signable_intent_json(
        intent_data,
        "sendTransaction",
        issuedAt,
        expiresAt
    );

    uint8_t hashed_to_sign[32];
    keccak256((const uint8_t*)to_sign, strlen(to_sign), hashed_to_sign);

    eoa_wallet_t w;
    if (!eoa_wallet_from_private_key_bytes(&w, wallet->seckey)) {

    }

    const char *sig = "";
    /*const char *sig = wallet_sign_message_hex_eip191(
        w.seckey,
        w.ctx,
        hashed_to_sign,
        sizeof(hashed_to_sign),
        NULL, 0);*/

    cJSON *intent_data_2 = sequence_build_contract_call_intent_json(
        identifier,
        chain_id_str,
        wallet->address,
        "contract_address",
        "contractCall",
        "0",
        "function_signature",
        "args[0].v.str",
        "args[1].v.str");

    char *intent_json = sequence_build_intent_json(
        intent_data_2,
        "sendTransaction",
        issuedAt,
        expiresAt,
        "wallet->session_id",
        sig
    );

	printf("Intent JSON: %s\n", intent_json);

    HttpResponse r = http_client_post_json(c, "/SendIntent", intent_json, 10000);

    if (r.error) {
        fprintf(stderr, "Request failed: %s\n", r.error);
        http_response_free(&r);
        http_client_destroy(c);
    }

	printf("ContractCall Response: %s\n", r.body);

	SequenceTransactionResult *result = sequence_build_transaction_intent_return(r.body);
	if (!result) {
		return "null";
	}

    return result->response.data.txHash;
}
