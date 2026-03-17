# Sequence C SDK

## Setup

#### Install dependencies

```shell
arch -arm64 brew install cmake
brew install secp256k1
```

For armcc support, install mbedtls via vcpkg.

```shell
// vcpkg
cd ~
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
sh bootstrap-vcpkg.sh
export PATH="$HOME/vcpkg:$PATH"

// Mbed TLS
// If necessary, add it to your CMAKE_PREFIX_PATH
vcpkg install mbedtls
```

cJSON

```shell
brew install cjson
```

#### Initialize the build directory

```shell
cmake -S . -B build
cmake --build build
```

#### Run the demo or cli

```shell
./build/sequence-demo
```

```shell
# Init
./build/sequence-cli init --access_key AQAAAAAAAKMyIkcpH4HUX6fFHcyNnjjSrak

# Get token balances
./build/sequence-cli get_token_balances --chain_id 137 --contract_address 0x3c499c542cEF5E3811e1192ce70d8cC03d5c3359 --wallet_address 0x8e3E38fe7367dd3b52D1e281E4e8400447C8d8B9 --include_metadata

# Sign in with Email
./build/sequence-cli sign_in_with_email --email andygruening@gmail.com

# Confirm Email sign in
./build/sequence-cli confirm_email_sign_in --email andygruening@gmail.com --code 123456

# Use wallet
./build/sequence-cli use_wallet --wallet_type test

# Create wallet
./build/sequence-cli create_wallet

# Sign message
./build/sequence-cli sign_message --chain_id 80002 --message test

# Send transaction
./build/sequence-cli send_transaction --chain_id 137 --to 0xE5E8B483FfC05967FcFed58cc98D053265af6D99 --value 1000
```

#### Homebrew setup

```shell
# Remove tap if needed
brew untap 0xsequence/c-sdk

# Create tap
brew tap 0xsequence/c-sdk https://github.com/0xsequence/c-sdk/

# Install tap
brew install 0xsequence/c-sdk/wallet
```

#### Homebrew installation

```shell
# Install
brew install 0xsequence/c-sdk/wallet

# Brew lists this package as 'wallet'
brew list -v wallet

# Uninstall
brew install 0xsequence/c-sdk/wallet
```
