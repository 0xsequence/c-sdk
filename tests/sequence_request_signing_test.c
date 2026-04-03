#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wallet/sequence_request_signing.h"
#include "wallet/requests/build_complete_auth_json.h"
#include "wallet/requests/build_sign_message_json.h"
#include "wallet/requests/build_send_transaction_json.h"
#include "evm/keccak256.h"
#include "utils/hex_utils.h"
#include "utils/string_utils.h"

static void expect_string(const char *label, const char *actual, const char *expected)
{
    if (!actual || strcmp(actual, expected) != 0) {
        fprintf(stderr, "%s mismatch\nexpected: %s\nactual:   %s\n",
            label,
            expected ? expected : "(null)",
            actual ? actual : "(null)");
        exit(1);
    }
}

static void test_sign_message_vector(void)
{
    static const uint8_t seckey[32] = {
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11
    };
    const char *wallet = "0x1234567890123456789012345678901234567890";
    const char *network = "amoy";
    const char *message = "hello";
    const char *nonce = "1710000000";
    const char *endpoint = "/SignMessage";
    const char *scope = "@1:test";
    const char *expected_payload =
        "{\"params\":{\"wallet\":\"0x1234567890123456789012345678901234567890\",\"network\":\"amoy\",\"message\":\"hello\"}}";
    const char *expected_preimage =
        "POST /rpc/Wallet/SignMessage\nnonce: 1710000000\n\n{\"params\":{\"wallet\":\"0x1234567890123456789012345678901234567890\",\"network\":\"amoy\",\"message\":\"hello\"}}";
    const char *expected_digest =
        "0x24b512b5aad6b77720d929914c135c81fa42879f21c3d1c6e86fa3cac4c18ca3";
    const char *expected_address =
        "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a";
    const char *expected_signature =
        "0x1ed397e17208e21f86bb8b87f00b6e85dc7cf00a999e0f735aafefe75b701f792a60894919590a142e55a4be4aa4fa58d9782702e38795660191080139a3ceda1b";
    const char *expected_header =
        "Authorization: Ethereum_Secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000000,sig=\"0x1ed397e17208e21f86bb8b87f00b6e85dc7cf00a999e0f735aafefe75b701f792a60894919590a142e55a4be4aa4fa58d9782702e38795660191080139a3ceda1b\"";

    char *payload = build_sign_message_json(wallet, network, message);
    char *preimage = sequence_build_wallet_request_preimage(endpoint, nonce, payload);
    char *digest = sequence_wallet_request_preimage_digest_hex(preimage);
    char *address = sequence_wallet_address_from_seckey(seckey);
    char *signature = sequence_sign_wallet_digest_hex_eip191(seckey, digest);
    char *signature_from_preimage = sequence_sign_wallet_request_preimage(seckey, preimage);
    char *header = sequence_build_wallet_authorization_header(scope, address, nonce, signature);

    expect_string("sign message payload", payload, expected_payload);
    expect_string("sign message preimage", preimage, expected_preimage);
    expect_string("sign message digest", digest, expected_digest);
    expect_string("sign message address", address, expected_address);
    expect_string("sign message signature", signature, expected_signature);
    expect_string("sign message signature from preimage", signature_from_preimage, expected_signature);
    expect_string("sign message header", header, expected_header);

    free(payload);
    free(preimage);
    free(digest);
    free(address);
    free(signature);
    free(signature_from_preimage);
    free(header);
}

static void test_send_transaction_vector(void)
{
    static const uint8_t seckey[32] = {
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11
    };
    const char *wallet = "0x1234567890123456789012345678901234567890";
    const char *network = "amoy";
    const char *to = "0xE5E8B483FfC05967FcFed58cc98D053265af6D99";
    const char *value = "1000";
    const char *nonce = "1710000001";
    const char *endpoint = "/SendTransaction";
    const char *scope = "@1:test";
    const char *expected_payload =
        "{\"params\":{\"mode\":\"Relayer\",\"wallet\":\"0x1234567890123456789012345678901234567890\",\"network\":\"amoy\",\"to\":\"0xE5E8B483FfC05967FcFed58cc98D053265af6D99\",\"value\":\"1000\"}}";
    const char *expected_preimage =
        "POST /rpc/Wallet/SendTransaction\nnonce: 1710000001\n\n{\"params\":{\"mode\":\"Relayer\",\"wallet\":\"0x1234567890123456789012345678901234567890\",\"network\":\"amoy\",\"to\":\"0xE5E8B483FfC05967FcFed58cc98D053265af6D99\",\"value\":\"1000\"}}";
    const char *expected_digest =
        "0xa38ffa5cde4c9830190b7c81c69fe4fbd6519eb7f53c348f2a9829cbfe11cb98";
    const char *expected_address =
        "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a";
    const char *expected_signature =
        "0xe4b227b6cb3cbd30ac636b06f97b9e44488d966ca0d49a257f9580477720881022085426548aabfc151d7ebfe0ad7271044d145c1c76cef6aeebeb67d520ae3d1c";
    const char *expected_header =
        "Authorization: Ethereum_Secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000001,sig=\"0xe4b227b6cb3cbd30ac636b06f97b9e44488d966ca0d49a257f9580477720881022085426548aabfc151d7ebfe0ad7271044d145c1c76cef6aeebeb67d520ae3d1c\"";

    char *payload = build_send_transaction_json(wallet, network, to, value);
    char *preimage = sequence_build_wallet_request_preimage(endpoint, nonce, payload);
    char *digest = sequence_wallet_request_preimage_digest_hex(preimage);
    char *address = sequence_wallet_address_from_seckey(seckey);
    char *signature = sequence_sign_wallet_digest_hex_eip191(seckey, digest);
    char *signature_from_preimage = sequence_sign_wallet_request_preimage(seckey, preimage);
    char *header = sequence_build_wallet_authorization_header(scope, address, nonce, signature);

    expect_string("send transaction payload", payload, expected_payload);
    expect_string("send transaction preimage", preimage, expected_preimage);
    expect_string("send transaction digest", digest, expected_digest);
    expect_string("send transaction address", address, expected_address);
    expect_string("send transaction signature", signature, expected_signature);
    expect_string("send transaction signature from preimage", signature_from_preimage, expected_signature);
    expect_string("send transaction header", header, expected_header);

    free(payload);
    free(preimage);
    free(digest);
    free(address);
    free(signature);
    free(signature_from_preimage);
    free(header);
}



static void test_complete_auth_vector(void)
{
    static const uint8_t seckey[32] = {
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11
    };
    const char *challenge = "challenge";
    const char *code = "123456";
    const char *verifier = "verifier-123";
    const char *nonce = "1710000002";
    const char *endpoint = "/CompleteAuth";
    const char *scope = "@1:test";
    const char *expected_answer =
        "0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd";
    const char *expected_payload =
        "{\"params\":{\"identityType\":\"Email\",\"authMode\":\"OTP\",\"verifier\":\"verifier-123\",\"answer\":\"0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd\"}}";
    const char *expected_preimage =
        "POST /rpc/Wallet/CompleteAuth\n"
        "nonce: 1710000002\n"
        "\n"
        "{\"params\":{\"identityType\":\"Email\",\"authMode\":\"OTP\",\"verifier\":\"verifier-123\",\"answer\":\"0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd\"}}";
    const char *expected_digest =
        "0x6fe84a6372290cd1e3b68276e1822dbb6021d7576bd6845387c62ee938e1274c";
    const char *expected_address =
        "0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a";
    const char *expected_signature =
        "0x051552b05b0ab8b4cf948803519e2dc63e8d7d0bc9a5637e59253d52eb6b1ca3301234e34441d67963f58015b40e8c43710a5edb1f2db451abbaa90b51a8c7871c";
    const char *expected_header =
        "Authorization: Ethereum_Secp256k1 scope=\"@1:test\",cred=\"0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a\",nonce=1710000002,sig=\"0x051552b05b0ab8b4cf948803519e2dc63e8d7d0bc9a5637e59253d52eb6b1ca3301234e34441d67963f58015b40e8c43710a5edb1f2db451abbaa90b51a8c7871c\"";

    uint8_t answer_digest[32];
    char *pre_hash_answer = concat_malloc(challenge, code);
    keccak256((const uint8_t *)pre_hash_answer, strlen(pre_hash_answer), answer_digest);
    char *answer = bytes_to_hex(answer_digest, sizeof(answer_digest));
    char *payload = sequence_build_complete_auth_json(verifier, answer);
    char *preimage = sequence_build_wallet_request_preimage(endpoint, nonce, payload);
    char *digest = sequence_wallet_request_preimage_digest_hex(preimage);
    char *address = sequence_wallet_address_from_seckey(seckey);
    char *signature = sequence_sign_wallet_digest_hex_eip191(seckey, digest);
    char *signature_from_preimage = sequence_sign_wallet_request_preimage(seckey, preimage);
    char *header = sequence_build_wallet_authorization_header(scope, address, nonce, signature);

    expect_string("complete auth answer", answer, expected_answer);
    expect_string("complete auth payload", payload, expected_payload);
    expect_string("complete auth preimage", preimage, expected_preimage);
    expect_string("complete auth digest", digest, expected_digest);
    expect_string("complete auth address", address, expected_address);
    expect_string("complete auth signature", signature, expected_signature);
    expect_string("complete auth signature from preimage", signature_from_preimage, expected_signature);
    expect_string("complete auth header", header, expected_header);

    free(pre_hash_answer);
    free(answer);
    free(payload);
    free(preimage);
    free(digest);
    free(address);
    free(signature);
    free(signature_from_preimage);
    free(header);
}

static void test_complete_auth_answer_hash_vector(void)
{
    const char *challenge = "challenge";
    const char *code = "123456";
    const char *verifier = "verifier-123";
    const char *expected_answer =
        "0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd";
    const char *expected_payload =
        "{\"params\":{\"identityType\":\"Email\",\"authMode\":\"OTP\",\"verifier\":\"verifier-123\",\"answer\":\"0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd\"}}";

    uint8_t digest[32];
    char *pre_hash_answer = concat_malloc(challenge, code);
    keccak256((const uint8_t *)pre_hash_answer, strlen(pre_hash_answer), digest);
    char *answer = bytes_to_hex(digest, sizeof(digest));
    char *payload = sequence_build_complete_auth_json(verifier, answer);

    expect_string("complete auth answer hash", answer, expected_answer);
    expect_string("complete auth payload", payload, expected_payload);

    free(pre_hash_answer);
    free(answer);
    free(payload);
}

int main(void)
{
    test_sign_message_vector();
    test_send_transaction_vector();
    test_complete_auth_vector();
    test_complete_auth_answer_hash_vector();
    printf("sequence_request_signing_test passed\n");
    return 0;
}
