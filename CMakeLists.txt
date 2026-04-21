cmake_minimum_required(VERSION 3.20)
project(sequence-c-sdk C)

set(CMAKE_C_STANDARD 99)

include(GNUInstallDirs)
include(CTest)

find_package(CURL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SECP256K1 REQUIRED libsecp256k1)
pkg_check_modules(CJSON REQUIRED IMPORTED_TARGET libcjson)
link_directories(${SECP256K1_LIBRARY_DIRS})

option(SEQUENCE_BUILD_DEMO "Build demo target" ON)
option(SEQUENCE_BUILD_CLI "Build CLI target" ON)

# ---- Library ----

set(SEQUENCE_SDK_SOURCES
        lib/generated/waas/waas.gen.c
        lib/runtime/sequence_runtime.c
        lib/wallet/sequence_auth.c
        lib/wallet/sequence_config.c
        lib/wallet/sequence_request_signing.c
        lib/wallet/sequence_wallet_ops.c
        lib/wallet/sequence_wallet_shared.c
        lib/networking/http_client.c
        lib/networking/http_client_common.c
        lib/infrastructure/is_valid_message_signature.c
        lib/indexer/get_token_balances.c
        lib/indexer/requests/get_token_balances_args.c
        lib/indexer/requests/get_token_balances_return.c
        lib/chains/chain_bindings.c
        lib/evm/eoa_wallet.c
        lib/evm/keccak256.c
        lib/evm/sign_message.c
        lib/utils/byte_utils.c
        lib/utils/globals.c
        lib/utils/hex_utils.c
        lib/utils/string_utils.c
        lib/utils/timestamps.c
        lib/utils/uuid.c
)

if(APPLE)
    list(APPEND SEQUENCE_SDK_SOURCES
            lib/storage/secure_storage.c
    )
elseif(UNIX)
    list(APPEND SEQUENCE_SDK_SOURCES
            lib/storage/secure_storage_posix.c
    )
else()
    message(FATAL_ERROR "sequence-c-sdk currently supports macOS and Linux only")
endif()

add_library(sequence_c_sdk ${SEQUENCE_SDK_SOURCES})

target_include_directories(sequence_c_sdk
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:include>
        ${SECP256K1_INCLUDE_DIRS}
        ${CJSON_INCLUDE_DIRS}
)

target_compile_options(sequence_c_sdk PUBLIC
        ${SECP256K1_CFLAGS_OTHER}
        ${CJSON_CFLAGS_OTHER}
)

target_link_libraries(sequence_c_sdk PUBLIC
        CURL::libcurl
        PkgConfig::CJSON
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

set(SEQUENCE_RUNTIME_TARGETS "")

if(SEQUENCE_BUILD_DEMO)
    add_executable(sequence-demo demo.c)
    target_link_libraries(sequence-demo PRIVATE sequence_c_sdk)
    list(APPEND SEQUENCE_RUNTIME_TARGETS sequence-demo)
endif()

# ---- CLI ----

if(SEQUENCE_BUILD_CLI)
    add_executable(sequence-wallet cli.c)
    target_link_libraries(sequence-wallet PRIVATE sequence_c_sdk)
    list(APPEND SEQUENCE_RUNTIME_TARGETS sequence-wallet)
endif()

# ---- Install ----

install(TARGETS sequence_c_sdk
        EXPORT sequence-c-sdk-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(SEQUENCE_RUNTIME_TARGETS)
    install(TARGETS ${SEQUENCE_RUNTIME_TARGETS}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

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

    if(UNIX AND NOT APPLE)
        add_executable(secure_storage_test
                tests/secure_storage_test.c)
        target_link_libraries(secure_storage_test PRIVATE sequence_c_sdk)
        add_test(NAME secure_storage_test COMMAND secure_storage_test)
    endif()
endif()
