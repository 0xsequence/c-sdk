# Request Signing Vectors

These vectors define the canonical pure signing flow extracted from the wallet
connector. They are intended to be reused across SDKs so other language
implementations can verify that they produce the same payload, preimage, digest,
signature, and authorization header.

All signatures below follow the current C SDK behavior:

1. Build the JSON payload as an unformatted single-line string.
2. Build the request preimage:
   ```text
   POST /rpc/Wallet{endpoint}
   nonce: {nonce}

   {payload}
   ```
3. Compute `keccak256(preimage)` and hex-encode it as a lowercase `0x...` string.
4. Sign that digest hex string as an EIP-191 UTF-8 message.
5. Build the authorization header:
   ```text
   Authorization: Ethereum_Secp256k1 scope="{scope}",cred="{address}",nonce={nonce},sig="{signature}"
   ```

## Shared Inputs

- Private key hex:
  `0x1111111111111111111111111111111111111111111111111111111111111111`
- Derived address:
  `0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a`
- Scope:
  `@1:test`

## Vector: CommitVerifier

- Inputs:
  - endpoint: `/CommitVerifier`
  - nonce: `1710000003`
  - identityType: `Email`
  - authMode: `OTP`
  - handle: `test@example.com`

- Expected payload:
  ```json
  {"identityType":"Email","authMode":"OTP","metadata":{},"handle":"test@example.com"}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/CommitVerifier
  nonce: 1710000003

  {"identityType":"Email","authMode":"OTP","metadata":{},"handle":"test@example.com"}
  ```

- Expected digest hex:
  `0x9dfdd24b22829750ea37aea91976359049d911f80bcebbc24f55093581915509`

- Expected signature:
  `0x48d263e63ce61f5b0095a85ed8c935694a4ad845553223b1953eaeec1f278aab1b5a65a4f8e331d335aea486a63deee684dfa4c62de5529c5fd03c6d356550131c`

- Expected authorization header:
  ```text
  Authorization: Ethereum_Secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000003,sig="0x48d263e63ce61f5b0095a85ed8c935694a4ad845553223b1953eaeec1f278aab1b5a65a4f8e331d335aea486a63deee684dfa4c62de5529c5fd03c6d356550131c"
  ```

## Vector: UseWallet

- Inputs:
  - endpoint: `/UseWallet`
  - nonce: `1710000004`
  - walletType: `Ethereum_EOA`
  - walletIndex: `0`

- Expected payload:
  ```json
  {"walletType":"Ethereum_EOA","walletIndex":0}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/UseWallet
  nonce: 1710000004

  {"walletType":"Ethereum_EOA","walletIndex":0}
  ```

- Expected digest hex:
  `0x71ab1786fdbae4975163cee47d3501ffb3e0076c426fbcfe1abfcb3bdd0e7ca8`

- Expected signature:
  `0x052b6dd4327e7e07bf31d2006fc1dd469f94a6024b3fa6e0cb1a1fc4dfb203d968682959b8fa01d5298c6c4dbb3ae0407f575fe7c1405535d38008d0b6d149551b`

- Expected authorization header:
  ```text
  Authorization: Ethereum_Secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000004,sig="0x052b6dd4327e7e07bf31d2006fc1dd469f94a6024b3fa6e0cb1a1fc4dfb203d968682959b8fa01d5298c6c4dbb3ae0407f575fe7c1405535d38008d0b6d149551b"
  ```

## Vector: CreateWallet

- Inputs:
  - endpoint: `/CreateWallet`
  - nonce: `1710000005`
  - walletType: `Ethereum_EOA`

- Expected payload:
  ```json
  {"walletType":"Ethereum_EOA"}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/CreateWallet
  nonce: 1710000005

  {"walletType":"Ethereum_EOA"}
  ```

- Expected digest hex:
  `0xef892d0808cdca87608ea5f59c1134b2d8e9f5979171f90bf5d01a70d45c8188`

- Expected signature:
  `0x9847f3dd071ca467441c15247ff445ac99911059fd043947ea90a59346f18c3800a3837f023ed2e31dc24f3d4581e287de1f10f015fbcf157df7fdc99c84f0921b`

- Expected authorization header:
  ```text
  Authorization: Ethereum_Secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000005,sig="0x9847f3dd071ca467441c15247ff445ac99911059fd043947ea90a59346f18c3800a3837f023ed2e31dc24f3d4581e287de1f10f015fbcf157df7fdc99c84f0921b"
  ```

## Vector: SignMessage

- Inputs:
  - endpoint: `/SignMessage`
  - nonce: `1710000000`
  - wallet: `0x1234567890123456789012345678901234567890`
  - network: `amoy`
  - message: `hello`

- Expected payload:
  ```json
  {"network":"amoy","wallet":"0x1234567890123456789012345678901234567890","message":"hello"}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/SignMessage
  nonce: 1710000000

  {"network":"amoy","wallet":"0x1234567890123456789012345678901234567890","message":"hello"}
  ```

- Expected digest hex:
  `0x17036192bfc9ba35197331ee39fc6774386a2dd49c2d47a49ae39e9b75dab65a`

- Expected signature:
  `0x33ea7a72ec2d69cd044f0d8cadcbde50aaf9c0e32288824bea74915549543c2e7dc81aa2fa69999d93f5bde0a65efa207aee634e0782f6e489ddcd73ef412eb51b`

- Expected authorization header:
  ```text
  Authorization: Ethereum_Secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000000,sig="0x33ea7a72ec2d69cd044f0d8cadcbde50aaf9c0e32288824bea74915549543c2e7dc81aa2fa69999d93f5bde0a65efa207aee634e0782f6e489ddcd73ef412eb51b"
  ```

## Vector: SendTransaction

- Inputs:
  - endpoint: `/SendTransaction`
  - nonce: `1710000001`
  - wallet: `0x1234567890123456789012345678901234567890`
  - network: `amoy`
  - to: `0xE5E8B483FfC05967FcFed58cc98D053265af6D99`
  - value: `1000`

- Expected payload:
  ```json
  {"network":"amoy","wallet":"0x1234567890123456789012345678901234567890","to":"0xE5E8B483FfC05967FcFed58cc98D053265af6D99","value":"1000","mode":"Relayer"}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/SendTransaction
  nonce: 1710000001

  {"network":"amoy","wallet":"0x1234567890123456789012345678901234567890","to":"0xE5E8B483FfC05967FcFed58cc98D053265af6D99","value":"1000","mode":"Relayer"}
  ```

- Expected digest hex:
  `0x0af11e533aafd6de32e5469bc719dea6b263322d759b48ba748915af20910399`

- Expected signature:
  `0xa5d41fb0a5ecc537b6e31f333bac9e13133bd0c144d18bc650b96d19a2e7804b4e87416cc2f92cf2a0b0fb142015de17d3084635f50760a6ea5463addea933f31b`

- Expected authorization header:
  ```text
  Authorization: Ethereum_Secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000001,sig="0xa5d41fb0a5ecc537b6e31f333bac9e13133bd0c144d18bc650b96d19a2e7804b4e87416cc2f92cf2a0b0fb142015de17d3084635f50760a6ea5463addea933f31b"
  ```

## Vector: CompleteAuth

- Inputs:
  - endpoint: `/CompleteAuth`
  - nonce: `1710000002`
  - challenge: `challenge`
  - code: `123456`
  - verifier: `verifier-123`

- Expected payload:
  ```json
  {"identityType":"Email","authMode":"OTP","verifier":"verifier-123","answer":"0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd"}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/CompleteAuth
  nonce: 1710000002

  {"identityType":"Email","authMode":"OTP","verifier":"verifier-123","answer":"0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd"}
  ```

- Expected digest hex:
  `0x804fc970f4bbec10e17544caeb9d643f1024ad1665d6ace7243422d78b60b0c8`

- Expected signature:
  `0xecd2af47b35ad15109d44309888787131c36c603d0a9500d7b4c1eaf33231d0c2c6d26270ee0fde89d037815697a08649fdb50a36916b3b35baf3613728e87e11b`

- Expected authorization header:
  ```text
  Authorization: Ethereum_Secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000002,sig="0xecd2af47b35ad15109d44309888787131c36c603d0a9500d7b4c1eaf33231d0c2c6d26270ee0fde89d037815697a08649fdb50a36916b3b35baf3613728e87e11b"
  ```

## Vector: CompleteAuth Answer Hash

- Inputs:
  - challenge: `challenge`
  - code: `123456`
  - verifier: `verifier-123`

- Expected answer hash:
  `0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd`

- Expected payload:
  ```json
  {"identityType":"Email","authMode":"OTP","verifier":"verifier-123","answer":"0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd"}
  ```
