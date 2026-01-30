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
cmake -S . -B cmake-build-debug
```

#### Build the executable

```shell
cmake --build cmake-build-debug --target sequence-c-sdk -j 12
```

#### Run the executable

```shell
./cmake-build-debug/sequence-c-sdk
```
