cmake_minimum_required(VERSION 3.20)
project(oms_wallet_c_sdk C)

set(CMAKE_C_STANDARD 99)

include(GNUInstallDirs)
include(CTest)

find_package(CURL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SECP256K1 REQUIRED libsecp256k1)
pkg_check_modules(CJSON REQUIRED IMPORTED_TARGET libcjson)
link_directories(${SECP256K1_LIBRARY_DIRS})

option(OMS_WALLET_BUILD_DEMO "Build demo target" ON)
option(OMS_WALLET_BUILD_CLI "Build CLI target" ON)

# ---- Library ----

set(OMS_WALLET_SDK_SOURCES
        lib/generated/waas/waas.gen.c
        lib/runtime/oms_wallet_runtime.c
        lib/wallet/oms_wallet_auth.c
        lib/wallet/oms_wallet_config.c
        lib/wallet/oms_wallet_request_signing.c
        lib/wallet/oms_wallet_ops.c
        lib/wallet/oms_wallet_shared.c
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
        lib/utils/base64url.c
        lib/utils/globals.c
        lib/utils/hex_utils.c
        lib/utils/sha256.c
        lib/utils/string_utils.c
        lib/utils/timestamps.c
        lib/utils/uuid.c
)

if(APPLE)
    list(APPEND OMS_WALLET_SDK_SOURCES
            lib/storage/secure_storage.c
    )
elseif(UNIX)
    list(APPEND OMS_WALLET_SDK_SOURCES
            lib/storage/secure_storage_posix.c
    )
else()
    message(FATAL_ERROR "oms-wallet-c-sdk currently supports macOS and Linux only")
endif()

add_library(oms_wallet_c_sdk ${OMS_WALLET_SDK_SOURCES})

target_include_directories(oms_wallet_c_sdk
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/oms-wallet>
        ${SECP256K1_INCLUDE_DIRS}
        ${CJSON_INCLUDE_DIRS}
)

target_compile_options(oms_wallet_c_sdk PUBLIC
        ${SECP256K1_CFLAGS_OTHER}
        ${CJSON_CFLAGS_OTHER}
)

target_link_libraries(oms_wallet_c_sdk PUBLIC
        CURL::libcurl
        PkgConfig::CJSON
        ${SECP256K1_LIBRARIES}
)

# macOS frameworks
if(APPLE)
    target_link_libraries(oms_wallet_c_sdk PUBLIC
            "-framework Security"
            "-framework CoreFoundation"
    )
endif()

# ---- Demo ----

set(OMS_WALLET_RUNTIME_TARGETS "")

if(OMS_WALLET_BUILD_DEMO)
    add_executable(oms_wallet_demo demo.c)
    target_link_libraries(oms_wallet_demo PRIVATE oms_wallet_c_sdk)
    set_target_properties(oms_wallet_demo PROPERTIES OUTPUT_NAME "oms-wallet-demo")
    list(APPEND OMS_WALLET_RUNTIME_TARGETS oms_wallet_demo)
endif()

# ---- CLI ----

if(OMS_WALLET_BUILD_CLI)
    add_executable(oms_wallet_cli cli.c)
    target_link_libraries(oms_wallet_cli PRIVATE oms_wallet_c_sdk)
    set_target_properties(oms_wallet_cli PROPERTIES OUTPUT_NAME "oms-wallet")
    list(APPEND OMS_WALLET_RUNTIME_TARGETS oms_wallet_cli)
endif()

# ---- Install ----

install(TARGETS oms_wallet_c_sdk
        EXPORT oms-wallet-c-sdk-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(OMS_WALLET_RUNTIME_TARGETS)
    install(TARGETS ${OMS_WALLET_RUNTIME_TARGETS}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(DIRECTORY lib/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/oms-wallet
        FILES_MATCHING PATTERN "*.h"
        PATTERN "oms_wallet_internal.h" EXCLUDE)

install(EXPORT oms-wallet-c-sdk-targets
        FILE oms-wallet-c-sdk-targets.cmake
        NAMESPACE oms_wallet::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/oms-wallet-c-sdk)

if(BUILD_TESTING)
    add_executable(oms_wallet_request_signing_test
            tests/oms_wallet_request_signing_test.c)
    target_link_libraries(oms_wallet_request_signing_test PRIVATE oms_wallet_c_sdk)
    add_test(NAME oms_wallet_request_signing_test COMMAND oms_wallet_request_signing_test)

    add_executable(timestamps_test
            tests/timestamps_test.c)
    target_link_libraries(timestamps_test PRIVATE oms_wallet_c_sdk)
    add_test(NAME timestamps_test COMMAND timestamps_test)

    if(UNIX AND NOT APPLE)
        add_executable(secure_storage_test
                tests/secure_storage_test.c)
        target_link_libraries(secure_storage_test PRIVATE oms_wallet_c_sdk)
        add_test(NAME secure_storage_test COMMAND secure_storage_test)
    endif()
endif()
