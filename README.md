# OmsWallet C SDK

## Support Matrix

- macOS: supported for development and local testing, uses Keychain-backed secure storage
- Linux: supported runtime target for POSIX + `libcurl` environments and for embedded integrations that provide their own transport/session/signer providers
- HTTP transport: built-in `libcurl` transport is optional; applications can provide an SDK-wide transport callback
- Session model: caller-owned `oms_wallet_sdk_t` context; each context owns config, session handles, transport, signer, and response limits

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
cmake -S . -B build -DOMS_WALLET_BUILD_DEMO=OFF -DOMS_WALLET_BUILD_CLI=OFF
cmake -S . -B build-no-curl -DOMS_WALLET_ENABLE_CURL_TRANSPORT=OFF -DOMS_WALLET_BUILD_DEMO=OFF -DOMS_WALLET_BUILD_CLI=OFF
```

`OMS_WALLET_BUILD_DEMO` and `OMS_WALLET_BUILD_CLI` only control those runtime
binaries. They do not disable the library or the test targets.

## Initialization

Allocate and initialize an `oms_wallet_sdk_t` before using wallet, indexer, or
API helpers. The SDK no longer uses process-global mutable config.

Minimal setup:

```c
#include <wallet/oms_wallet_config.h>

oms_wallet_sdk_t sdk;
if (oms_wallet_sdk_init(&sdk, "YOUR_ACCESS_KEY") != 0) {
    /* handle config initialization failure */
}
```

Common overrides:

```c
oms_wallet_config_set_indexer_url_template(&sdk, "https://dev-{value}-indexer.sequence.app/rpc/Indexer/");
oms_wallet_config_set_api_rpc_url(&sdk, "https://dev-api.sequence.app/rpc/API");
oms_wallet_config_set_wallet_rpc_url(&sdk, "https://your-wallet-host/rpc/Wallet");
oms_wallet_config_set_origin_header(&sdk, "Origin: http://localhost:3000");
oms_wallet_config_set_storage_dir(&sdk, "/var/lib/oms-wallet-c-sdk");
oms_wallet_config_set_max_response_bytes(&sdk, 64 * 1024);
```

The default wallet auth scope is `proj_1`. Only override it if your environment
requires a different scope.

Call `oms_wallet_sdk_cleanup(&sdk)` when you are done with SDK-owned state.

`oms_wallet_restore_session(&sdk)` returns:

- `1` when a usable saved session was restored
- `0` when no usable saved session exists yet
- `-1` on an actual storage or restore failure

## Linux Secure Storage

By default, the SDK creates a local `ethereum-secp256k1` auth credential used
to sign WaaS RPC requests. Applications that need hardware-backed credentials
can replace this with `oms_wallet_config_set_auth_signer_provider(&sdk, ...)`. Custom
providers own credential creation, credential identity, and request signing; the
SDK does not require providers to export private key bytes.

The SDK owns WaaS authorization canonicalization and passes canonical message
bytes to the provider. Providers should sign those bytes as-is and return the
signature hex string; they should not rebuild the HTTP request preimage
independently. Tests and embedded integrations can also replace all SDK
networking with `oms_wallet_config_set_transport(&sdk, ...)`. Wallet RPC
requests are delivered to that transport after the SDK appends Authorization.
Indexer and API helper requests use the same transport boundary.

Session persistence can be platform-owned too:
`oms_wallet_config_set_session_store_provider(&sdk, ...)` lets applications
store challenge/verifier/signer-id/wallet-id state in NVRAM, TPM-sealed blobs,
an encrypted database, or product-specific storage.

On Linux, the default software provider stores its auth credential with a
file-per-key backend using:

- directory permissions: `0700`
- file permissions: `0600`
- atomic replace on write

This Linux backend is permission-protected filesystem storage. It is not
encrypted at rest like macOS Keychain storage.

Default storage path:

- `$HOME/.oms-wallet-c-sdk` when `HOME` is set
- `./.oms-wallet-c-sdk` otherwise

Override the storage location with:

```c
oms_wallet_config_set_storage_dir(&sdk, "/path/to/app-state");
```

Persisted session keys used by the SDK:

- `access-key`: CLI-stored access key used to initialize SDK config
- `oms_auth_signer_id`: opaque auth credential signer handle
- `seckey`: internal raw auth credential used only by the default software signer
- `challenge`: pending email sign-in challenge
- `verifier`: pending email sign-in verifier
- `oms_wallet_id`: selected wallet id for follow-up wallet operations
- `oms_wallet_address`: last selected wallet address

#### Run tests

```shell
ctest --test-dir build --output-on-failure

# Focused request-signing parity test
./build/oms_wallet_request_signing_test
```

Current test coverage:

- `oms_wallet_request_signing_test`: validates canonical request payloads, preimages, digests, signatures, and authorization headers against checked-in vectors
- `timestamps_test`: validates nonce monotonicity from `timestamp_next_nonce()`
- `secure_storage_test`: Linux-only regression test for the POSIX secure-storage backend
- `auth_signer_provider_test`: validates custom signer provider ownership and restore flow
- `embedded_secure_auth_provider_test`: simulates an embedded secure-element signer plus SDK-owned request canonicalization over a custom transport

#### Run the demo or cli

```shell
./build/oms-wallet-demo
```

```shell
# Init
./build/oms-wallet init --access-key AQAAAAAAAAK2JvvZhWqZ51riasWBftkrVXE

# Get token balances
./build/oms-wallet get-token-balances --chain-id 137 --contract-address 0x3c499c542cEF5E3811e1192ce70d8cC03d5c3359 --wallet-address 0x8e3E38fe7367dd3b52D1e281E4e8400447C8d8B9 --include-metadata

# Sign in with Email
./build/oms-wallet sign-in-with-email --email andygruening@gmail.com

# Confirm Email sign in and automatically select the first matching wallet,
# or create one if none exists for that type
./build/oms-wallet confirm-email-sign-in --code 123456 --wallet-type ethereum

# Or, omit --wallet-type to list the available wallets returned by CompleteAuth
./build/oms-wallet confirm-email-sign-in --code 123456

# Optional: switch to a specific wallet later
./build/oms-wallet use-wallet --wallet-id <wallet-id>

# Optional: create an additional wallet of the default type (ethereum)
./build/oms-wallet create-wallet

# Sign message
./build/oms-wallet sign-message --chain-id 80002 --message test

# Verify signature
./build/oms-wallet verify-signature --chain-id 80002 --wallet-address 0xb7461CcfFfc7378747C6f82804E4dEc04b9E6148 --message test --signature 0x...

# Send transaction
./build/oms-wallet send-transaction --chain-id 80002 --to 0xE5E8B483FfC05967FcFed58cc98D053265af6D99 --value 0
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
