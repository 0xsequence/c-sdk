cmake_minimum_required(VERSION 4.1)
project(sequence-c-sdk C)

set(CMAKE_C_STANDARD 99)

add_executable(sequence-c-sdk main.c
        lib/networking/http_client.h
        lib/networking/http_client.c
        lib/embedded-wallet/sequence_connector.c
        lib/embedded-wallet/sequence_connector.h
        lib/embedded-wallet/sequence_connector.c
        lib/embedded-wallet/sequence_config.h
        lib/embedded-wallet/sequence_config.c
        lib/networking/http_client_mbed.h
        lib/networking/http_client_mbed.c
        lib/indexer/get_token_balances.h
        lib/indexer/get_token_balances.c
        lib/chains/chain_bindings.h
        lib/chains/chain_bindings.c
        lib/storage/storage_handler.h
        lib/storage/storage_handler.c
        lib/indexer/requests/get_token_balances_args.h
        lib/indexer/requests/get_token_balances_args.c
        lib/indexer/models/page.h
        lib/indexer/models/balance.h
        lib/indexer/requests/get_token_balances_return.h
        lib/indexer/requests/get_token_balances_return.c
        lib/utils/timestamps.h
        lib/evm/keccak256.h
        lib/evm/keccak256.c
        lib/evm/sign_message.h
        lib/evm/sign_message.c
        lib/evm/eoa_wallet.h
        lib/evm/eoa_wallet.c
        lib/utils/hex_utils.h
        lib/utils/hex_utils.c
        lib/utils/timestamps.c
        lib/utils/byte_utils.c
        lib/utils/byte_utils.h
        lib/utils/string_utils.h
        lib/utils/string_utils.c
        lib/embedded-wallet/requests/build_complete_auth_json.c
        lib/embedded-wallet/requests/build_complete_auth_json.h
        lib/embedded-wallet/requests/build_commit_verifier_json.c
        lib/embedded-wallet/requests/build_commit_verifier_json.h
        lib/embedded-wallet/requests/build_sign_message_json.c
        lib/embedded-wallet/requests/build_sign_message_json.h
        lib/embedded-wallet/requests/build_use_wallet_json.c
        lib/embedded-wallet/requests/build_use_wallet_json.h
        lib/embedded-wallet/requests/build_send_transaction_json.c
        lib/embedded-wallet/requests/build_send_transaction_json.h
        lib/embedded-wallet/requests/wallet_return.c
        lib/embedded-wallet/requests/wallet_return.h
        lib/embedded-wallet/requests/send_transaction_return.c
        lib/embedded-wallet/requests/send_transaction_return.h
        lib/embedded-wallet/requests/sign_message_return.c
        lib/embedded-wallet/requests/sign_message_return.h
        lib/embedded-wallet/requests/build_create_wallet_json.c
        lib/embedded-wallet/requests/build_create_wallet_json.h
        lib/embedded-wallet/requests/commit_verifier_return.c
        lib/embedded-wallet/requests/commit_verifier_return.h
        lib/embedded-wallet/requests/complete_auth_return.c
        lib/embedded-wallet/requests/complete_auth_return.h
        lib/embedded-wallet/sequence_wallet.h
        lib/embedded-wallet/sequence_wallet.c
        lib/utils/uuid.h
        lib/utils/uuid.c
        lib/utils/globals.h
        lib/utils/globals.c
)

target_include_directories(sequence-c-sdk PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

# ---- MBed TLS ----

find_package(MbedTLS CONFIG REQUIRED)

get_property(_imported DIRECTORY PROPERTY IMPORTED_TARGETS)
message(STATUS "Imported targets: ${_imported}")

target_link_libraries(sequence-c-sdk PRIVATE
        MbedTLS::mbedtls
        MbedTLS::mbedx509
        MbedTLS::tfpsacrypto
)

# ---- CURL ----
find_package(CURL REQUIRED)
target_link_libraries(sequence-c-sdk PRIVATE CURL::libcurl)

# ---- cJSON (Homebrew) ----
set(HOMEBREW_PREFIX "/opt/homebrew" CACHE PATH "Homebrew prefix")

find_path(CJSON_INCLUDE_DIR
        NAMES cjson/cJSON.h
        PATHS
        ${HOMEBREW_PREFIX}/include
        /usr/local/include
)

find_library(CJSON_LIBRARY
        NAMES cjson
        PATHS
        ${HOMEBREW_PREFIX}/lib
        /usr/local/lib
)

if (NOT CJSON_INCLUDE_DIR OR NOT CJSON_LIBRARY)
    message(FATAL_ERROR "Could not find cJSON in Homebrew. Try: brew install cjson")
endif()

target_include_directories(sequence-c-sdk PRIVATE ${CJSON_INCLUDE_DIR})
target_link_libraries(sequence-c-sdk PRIVATE ${CJSON_LIBRARY})

# ---- libsecp256k1 ----
find_package(PkgConfig REQUIRED)
pkg_check_modules(SECP256K1 REQUIRED libsecp256k1)

target_link_directories(sequence-c-sdk PRIVATE ${SECP256K1_LIBRARY_DIRS})

target_include_directories(sequence-c-sdk PRIVATE
        ${SECP256K1_INCLUDE_DIRS}
)

target_link_libraries(sequence-c-sdk PRIVATE
        ${SECP256K1_LIBRARIES}
)

target_compile_options(sequence-c-sdk PRIVATE
        ${SECP256K1_CFLAGS_OTHER}
)
