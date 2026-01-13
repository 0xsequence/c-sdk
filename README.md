# Sequence C SDK

## Setup

#### Install CMake

```shell
arch -arm64 brew install cmake
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
