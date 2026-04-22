# Sequence C SDK

## Support Matrix

- macOS: supported for development and local testing, uses Keychain-backed secure storage
- Linux: supported runtime target for normal POSIX + `libcurl` environments, uses POSIX file-backed secure storage
- HTTP transport: `libcurl` only
- Session model: single-session and not thread-safe

## Setup

#### Install dependencies

```shell
arch -arm64 brew install cmake pkg-config
brew install secp256k1 cjson curl
```

On Linux, install the equivalent development packages for `cmake`, `pkg-config`,
`libcurl`, `libcjson`, and `libsecp256k1`.

Debian / Ubuntu example:

```shell
apt-get update
apt-get install -y build-essential cmake pkg-config libcurl4-openssl-dev libcjson-dev libsecp256k1-dev
```

#### Initialize the build directory

```shell
cmake -S . -B build
cmake --build build
```

Optional targets:

```shell
cmake -S . -B build -DSEQUENCE_BUILD_DEMO=OFF -DSEQUENCE_BUILD_CLI=OFF
```

`SEQUENCE_BUILD_DEMO` and `SEQUENCE_BUILD_CLI` only control those runtime
binaries. They do not disable the library or the test targets.

## Initialization

Call `sequence_config_init(...)` before using wallet, indexer, or API helpers.
The SDK no longer falls back to built-in runtime URLs when config is unset.

Minimal setup:

```c
#include "wallet/sequence_config.h"

if (sequence_config_init("YOUR_ACCESS_KEY") != 0) {
    /* handle config initialization failure */
}
```

Common overrides:

```c
sequence_config_set_indexer_url_template("https://dev-{value}-indexer.sequence.app/rpc/Indexer/");
sequence_config_set_api_rpc_url("https://dev-api.sequence.app/rpc/API");
sequence_config_set_wallet_rpc_url("https://your-wallet-host/rpc/Wallet");
sequence_config_set_wallet_auth_scope("@1:test");
sequence_config_set_origin_header("Origin: http://localhost:3000");
sequence_config_set_storage_dir("/var/lib/sequence-c-sdk");
```

Call `sequence_config_cleanup()` when you are done with SDK-owned config state.

## Linux Secure Storage

On Linux, secure storage uses a file-per-key backend with:

- directory permissions: `0700`
- file permissions: `0600`
- atomic replace on write

Default storage path:

- `$HOME/.sequence-c-sdk` when `HOME` is set
- `./.sequence-c-sdk` otherwise

Override the storage location with:

```c
sequence_config_set_storage_dir("/path/to/app-state");
```

#### Temporary generated-client patch

The vendored generated WAAS C client currently includes a small local
compatibility patch in `lib/generated/waas/waas.gen.c` to tolerate a missing
`iss` field in the live `CompleteAuth` response.

This is temporary and should be removed once the WAAS API contract is updated
or fixed upstream.

#### Run tests

```shell
ctest --test-dir build --output-on-failure

# Focused request-signing parity test
./build/sequence_request_signing_test
```

Current test coverage:

- `sequence_request_signing_test`: validates canonical request payloads, preimages, digests, signatures, and authorization headers against checked-in vectors
- `timestamps_test`: validates nonce monotonicity from `timestamp_next_nonce()`

#### Run the demo or cli

```shell
./build/sequence-demo
```

```shell
# Init
./build/sequence-wallet init --access-key AQAAAAAAAAK2JvvZhWqZ51riasWBftkrVXE

# Get token balances
./build/sequence-wallet get-token-balances --chain-id 137 --contract-address 0x3c499c542cEF5E3811e1192ce70d8cC03d5c3359 --wallet-address 0x8e3E38fe7367dd3b52D1e281E4e8400447C8d8B9 --include-metadata

# Sign in with Email
./build/sequence-wallet sign-in-with-email --email andygruening@gmail.com

# Confirm Email sign in
./build/sequence-wallet confirm-email-sign-in --code 123456 --wallet-type ethereum

# Use wallet
./build/sequence-wallet use-wallet --wallet-id <wallet-id>

# Create wallet
./build/sequence-wallet create-wallet

# Sign message
./build/sequence-wallet sign-message --chain-id 80002 --message test

# Verify signature
./build/sequence-wallet verify-signature --chain-id 80002 --wallet-address 0xb7461CcfFfc7378747C6f82804E4dEc04b9E6148 --message test --signature 0x...

# Send transaction
./build/sequence-wallet send-transaction --chain-id 80002 --to 0xE5E8B483FfC05967FcFed58cc98D053265af6D99 --value 0
```

#### Homebrew version release

1. Create a tag and version release on GitHub
2. Reference the .tar.gz file `url` from the release inside the `Formula/wallet.rb` file
3. Compute the file hash `curl -L <url.tar.gz> | shasum -a 256`
4. Replace the sha256 value inside `wallet.rb` with the hash from the latest release
5. Create a PR and merge it into `master`

#### Homebrew installation

Homebrew is the macOS CLI distribution path.

```shell
# Remove tap if needed
brew untap 0xsequence/c-sdk

# Create tap, then install
brew tap 0xsequence/c-sdk https://github.com/0xsequence/c-sdk/
brew install 0xsequence/c-sdk/wallet
```
