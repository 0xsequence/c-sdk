cmake_minimum_required(VERSION 3.20)
project(sequence-c-sdk C)

set(CMAKE_C_STANDARD 99)

include(GNUInstallDirs)
include(CTest)

find_package(MbedTLS CONFIG REQUIRED)
find_package(CURL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SECP256K1 REQUIRED libsecp256k1)
pkg_check_modules(JSON_C REQUIRED IMPORTED_TARGET json-c)
link_directories(${SECP256K1_LIBRARY_DIRS})

# ---- Library ----

add_library(sequence_c_sdk
        lib/generated/waas/waas.gen.c
        lib/networking/http_client.c
        lib/wallet/sequence_auth.c
        lib/wallet/sequence_config.c
        lib/wallet/sequence_request_signing.c
        lib/wallet/sequence_wallet_ops.c
        lib/wallet/sequence_wallet_shared.c
        lib/networking/http_client_mbed.c
        lib/infrastructure/is_valid_message_signature.c
        lib/indexer/get_token_balances.c
        lib/chains/chain_bindings.c
        lib/storage/storage_handler.c
        lib/storage/secure_storage.c
        lib/indexer/requests/get_token_balances_args.c
        lib/indexer/requests/get_token_balances_return.c
        lib/evm/keccak256.c
        lib/evm/sign_message.c
        lib/evm/eoa_wallet.c
        lib/utils/hex_utils.c
        lib/utils/timestamps.c
        lib/utils/byte_utils.c
        lib/utils/string_utils.c
        lib/utils/uuid.c
        lib/utils/globals.c
        lib/storage/secure_storage.c
        lib/storage/secure_storage.h
)

target_include_directories(sequence_c_sdk
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:include>
        ${SECP256K1_INCLUDE_DIRS}
)

target_compile_options(sequence_c_sdk PUBLIC
        ${SECP256K1_CFLAGS_OTHER}
)

target_link_libraries(sequence_c_sdk PUBLIC
        MbedTLS::mbedtls
        MbedTLS::mbedx509
        MbedTLS::tfpsacrypto
        CURL::libcurl
        PkgConfig::JSON_C
        ${SECP256K1_LIBRARIES}
)

# macOS frameworks
if(APPLE)
    target_link_libraries(sequence_c_sdk PUBLIC
            "-framework Security"
            "-framework CoreFoundation"
    )
endif()

# ---- Demo ----

add_executable(sequence-demo demo.c)
target_link_libraries(sequence-demo PRIVATE sequence_c_sdk)

# ---- CLI ----

add_executable(sequence-wallet cli.c)
target_link_libraries(sequence-wallet PRIVATE sequence_c_sdk)

# ---- Install ----

install(TARGETS sequence_c_sdk
        EXPORT sequence-c-sdk-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(TARGETS sequence-demo sequence-wallet
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY lib/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sequence
        FILES_MATCHING PATTERN "*.h"
        PATTERN "sequence_wallet_internal.h" EXCLUDE)

install(EXPORT sequence-c-sdk-targets
        FILE sequence-c-sdk-targets.cmake
        NAMESPACE sequence::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sequence-c-sdk)

if(BUILD_TESTING)
    add_executable(sequence_request_signing_test
            tests/sequence_request_signing_test.c)
    target_link_libraries(sequence_request_signing_test PRIVATE sequence_c_sdk)
    add_test(NAME sequence_request_signing_test COMMAND sequence_request_signing_test)

    add_executable(timestamps_test
            tests/timestamps_test.c)
    target_link_libraries(timestamps_test PRIVATE sequence_c_sdk)
    add_test(NAME timestamps_test COMMAND timestamps_test)
endif()
