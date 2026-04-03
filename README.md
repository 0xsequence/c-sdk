# Sequence C SDK

## Setup

#### Install dependencies

```shell
arch -arm64 brew install cmake pkg-config
brew install secp256k1 cjson curl mbedtls
```

`mbedtls` is required by the current CMake build. For standard macOS setup, install it with Homebrew.
If you are building with `armcc` / Arm Compiler, you can also provide `mbedtls` via `vcpkg`:

```shell
cd ~
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
sh bootstrap-vcpkg.sh
export PATH="$HOME/vcpkg:$PATH"

# If necessary, add it to your CMAKE_PREFIX_PATH
vcpkg install mbedtls
```

#### Initialize the build directory

```shell
cmake -S . -B build
cmake --build build
```

#### Run tests

```shell
ctest --test-dir build --output-on-failure

# Focused request-signing parity test
./build/sequence_request_signing_test
```

#### Run the demo or cli

```shell
./build/sequence-demo
```

```shell
# Init
./build/sequence-cli init --access_key AQAAAAAAAAK2JvvZhWqZ51riasWBftkrVXE

# Get token balances
./build/sequence-cli get_token_balances --chain_id 137 --contract_address 0x3c499c542cEF5E3811e1192ce70d8cC03d5c3359 --wallet_address 0x8e3E38fe7367dd3b52D1e281E4e8400447C8d8B9 --include_metadata

# Sign in with Email
./build/sequence-cli sign_in_with_email --email andygruening@gmail.com

# Confirm Email sign in
./build/sequence-cli confirm_email_sign_in --email andygruening@gmail.com --code 123456

# Use wallet
./build/sequence-cli use_wallet --wallet_type Ethereum_EOA

# Create wallet
./build/sequence-cli create_wallet

# Sign message
./build/sequence-cli sign_message --chain_id 80002 --message test

# Verify signature
./build/sequence-cli verify_signature --chain_id 80002 --wallet_address 0xb7461CcfFfc7378747C6f82804E4dEc04b9E6148 --message test --signature 0x...

# Send transaction
./build/sequence-cli send_transaction --chain_id 80002 --to 0xE5E8B483FfC05967FcFed58cc98D053265af6D99 --value 0
```

#### Homebrew version release

1. Create a tag and version release on GitHub
2. Reference the .tar.gz file `url` from the release inside the `Formula/wallet.rb` file
3. Compute the file hash `curl -L <url.tar.gz> | shasum -a 256`
4. Replace the sha256 value inside `wallet.rb` with the hash from the latest release
5. Create a PR and merge it into `master`

#### Homebrew installation

```shell
# Remove tap if needed
brew untap 0xsequence/c-sdk

# Create tap, then install
brew tap 0xsequence/c-sdk https://github.com/0xsequence/c-sdk/
brew install 0xsequence/c-sdk/wallet
```
