cmake_minimum_required(VERSION 4.1)
project(sequence-c-sdk C)

set(CMAKE_C_STANDARD 99)

add_executable(sequence-c-sdk main.c
        lib/networking/http_client.h
        lib/networking/http_client.c
        lib/evm/eoa_wallet.cpp
        lib/evm/eoa_wallet.h
        lib/evm/eoa_wallet.h
        lib/embedded-wallet/sequence_wallet.h
        lib/embedded-wallet/sequence_wallet.c
        lib/embedded-wallet/sequence_login.c
        lib/embedded-wallet/sequence_login.h)

target_include_directories(sequence-c-sdk PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

# ---- CURL ----
find_package(CURL REQUIRED)
target_link_libraries(sequence-c-sdk PRIVATE CURL::libcurl)

# ---- OpenSSL (for SHA-256, cleansing, etc.) ----
find_package(OpenSSL REQUIRED)
target_link_libraries(sequence-c-sdk PRIVATE OpenSSL::Crypto)

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
