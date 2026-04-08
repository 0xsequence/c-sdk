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
  {"params":{"identityType":"Email","authMode":"OTP","metadata":{},"handle":"test@example.com"}}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/CommitVerifier
  nonce: 1710000003

  {"params":{"identityType":"Email","authMode":"OTP","metadata":{},"handle":"test@example.com"}}
  ```

- Expected digest hex:
  `0xf39c10b9784a7d291c58b6f53136c014985c90101a08d8b9b3531a4ec90c672f`

- Expected signature:
  `0x1f4c5dc95a2c943b61142bd1a839c92e05ea23e80f6b94b58162948ae9a64a467a46a683fb8daadb69b730c1b7aa4fa3cf4f5812de793273fcf40060d8bc3da01c`

- Expected authorization header:
  ```text
  Authorization: Ethereum_Secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000003,sig="0x1f4c5dc95a2c943b61142bd1a839c92e05ea23e80f6b94b58162948ae9a64a467a46a683fb8daadb69b730c1b7aa4fa3cf4f5812de793273fcf40060d8bc3da01c"
  ```

## Vector: UseWallet

- Inputs:
  - endpoint: `/UseWallet`
  - nonce: `1710000004`
  - walletType: `Ethereum_EOA`
  - walletIndex: `0`

- Expected payload:
  ```json
  {"params":{"walletType":"Ethereum_EOA","walletIndex":0}}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/UseWallet
  nonce: 1710000004

  {"params":{"walletType":"Ethereum_EOA","walletIndex":0}}
  ```

- Expected digest hex:
  `0x28157674c8911273678a3eb23284d730b3c899883be52886104379c187b06ca6`

- Expected signature:
  `0xcfb3f9ead0541191b568421dd3b46e8544e141d2fa2fcb5e56a99aefb82a564f5b0846412f50ff711aac9051b144d322fac31a8e59181891136eaf2667c80f891c`

- Expected authorization header:
  ```text
  Authorization: Ethereum_Secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000004,sig="0xcfb3f9ead0541191b568421dd3b46e8544e141d2fa2fcb5e56a99aefb82a564f5b0846412f50ff711aac9051b144d322fac31a8e59181891136eaf2667c80f891c"
  ```

## Vector: CreateWallet

- Inputs:
  - endpoint: `/CreateWallet`
  - nonce: `1710000005`
  - walletType: `Ethereum_EOA`

- Expected payload:
  ```json
  {"params":{"walletType":"Ethereum_EOA"}}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/CreateWallet
  nonce: 1710000005

  {"params":{"walletType":"Ethereum_EOA"}}
  ```

- Expected digest hex:
  `0x1c8d8e5af6e44c973c6619218ebcf89560e59cb4f775acd12b875e2442e98f6f`

- Expected signature:
  `0xff6cee523cfd26cc547afc5d9d8961ba025392095884b11333abdc4cdc0e26ac16eb955cea2c444c479fb9ea011a728dd1509049c3744cd3c038bc3a51a714811b`

- Expected authorization header:
  ```text
  Authorization: Ethereum_Secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000005,sig="0xff6cee523cfd26cc547afc5d9d8961ba025392095884b11333abdc4cdc0e26ac16eb955cea2c444c479fb9ea011a728dd1509049c3744cd3c038bc3a51a714811b"
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
  {"params":{"network":"amoy","wallet":"0x1234567890123456789012345678901234567890","message":"hello"}}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/SignMessage
  nonce: 1710000000

  {"params":{"network":"amoy","wallet":"0x1234567890123456789012345678901234567890","message":"hello"}}
  ```

- Expected digest hex:
  `0x1da9b6e65c2472c77b51667e01e60268e10215073177cc7d7f192b0fdbb415ec`

- Expected signature:
  `0x16ff2ad055498874a0531874821fcaa687168fbc4402e5d446592888b2c29c7b1b968cb1316228c4fc0164e2af60d211947beb5a1ee1e84c20d36c59522269251c`

- Expected authorization header:
  ```text
  Authorization: Ethereum_Secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000000,sig="0x16ff2ad055498874a0531874821fcaa687168fbc4402e5d446592888b2c29c7b1b968cb1316228c4fc0164e2af60d211947beb5a1ee1e84c20d36c59522269251c"
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
  {"params":{"network":"amoy","wallet":"0x1234567890123456789012345678901234567890","to":"0xE5E8B483FfC05967FcFed58cc98D053265af6D99","value":"1000","mode":"Relayer"}}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/SendTransaction
  nonce: 1710000001

  {"params":{"network":"amoy","wallet":"0x1234567890123456789012345678901234567890","to":"0xE5E8B483FfC05967FcFed58cc98D053265af6D99","value":"1000","mode":"Relayer"}}
  ```

- Expected digest hex:
  `0x9ba4e8c8a581eb0330ef48a1cf6bad11009db74c3d6bf9e913799416d0e00305`

- Expected signature:
  `0xa7ea45d8349c3cdb5c5a4a78937120048c4711cb2e12bf13725423b391d6733e5c0080dbc697fd12a08c5c4faa7bb5182f7f6f956597e1f437b5ec9bbaa164ae1c`

- Expected authorization header:
  ```text
  Authorization: Ethereum_Secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000001,sig="0xa7ea45d8349c3cdb5c5a4a78937120048c4711cb2e12bf13725423b391d6733e5c0080dbc697fd12a08c5c4faa7bb5182f7f6f956597e1f437b5ec9bbaa164ae1c"
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
  {"params":{"identityType":"Email","authMode":"OTP","verifier":"verifier-123","answer":"0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd"}}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/CompleteAuth
  nonce: 1710000002

  {"params":{"identityType":"Email","authMode":"OTP","verifier":"verifier-123","answer":"0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd"}}
  ```

- Expected digest hex:
  `0x6fe84a6372290cd1e3b68276e1822dbb6021d7576bd6845387c62ee938e1274c`

- Expected signature:
  `0x051552b05b0ab8b4cf948803519e2dc63e8d7d0bc9a5637e59253d52eb6b1ca3301234e34441d67963f58015b40e8c43710a5edb1f2db451abbaa90b51a8c7871c`

- Expected authorization header:
  ```text
  Authorization: Ethereum_Secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000002,sig="0x051552b05b0ab8b4cf948803519e2dc63e8d7d0bc9a5637e59253d52eb6b1ca3301234e34441d67963f58015b40e8c43710a5edb1f2db451abbaa90b51a8c7871c"
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
  {"params":{"identityType":"Email","authMode":"OTP","verifier":"verifier-123","answer":"0x752c0acc530a06ddbccae9295f7fd287037f7e2c19272c7506adce3175075fdd"}}
  ```
