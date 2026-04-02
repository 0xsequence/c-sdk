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

## Vector: SignMessage

- Inputs:
  - endpoint: `/SignMessage`
  - nonce: `1710000000`
  - wallet: `0x1234567890123456789012345678901234567890`
  - network: `amoy`
  - message: `hello`

- Expected payload:
  ```json
  {"params":{"wallet":"0x1234567890123456789012345678901234567890","network":"amoy","message":"hello"}}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/SignMessage
  nonce: 1710000000

  {"params":{"wallet":"0x1234567890123456789012345678901234567890","network":"amoy","message":"hello"}}
  ```

- Expected digest hex:
  `0x24b512b5aad6b77720d929914c135c81fa42879f21c3d1c6e86fa3cac4c18ca3`

- Expected signature:
  `0x1ed397e17208e21f86bb8b87f00b6e85dc7cf00a999e0f735aafefe75b701f792a60894919590a142e55a4be4aa4fa58d9782702e38795660191080139a3ceda1b`

- Expected authorization header:
  ```text
  Authorization: Ethereum_Secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000000,sig="0x1ed397e17208e21f86bb8b87f00b6e85dc7cf00a999e0f735aafefe75b701f792a60894919590a142e55a4be4aa4fa58d9782702e38795660191080139a3ceda1b"
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
  {"params":{"mode":"Relayer","wallet":"0x1234567890123456789012345678901234567890","network":"amoy","to":"0xE5E8B483FfC05967FcFed58cc98D053265af6D99","value":"1000"}}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/SendTransaction
  nonce: 1710000001

  {"params":{"mode":"Relayer","wallet":"0x1234567890123456789012345678901234567890","network":"amoy","to":"0xE5E8B483FfC05967FcFed58cc98D053265af6D99","value":"1000"}}
  ```

- Expected digest hex:
  `0xa38ffa5cde4c9830190b7c81c69fe4fbd6519eb7f53c348f2a9829cbfe11cb98`

- Expected signature:
  `0xe4b227b6cb3cbd30ac636b06f97b9e44488d966ca0d49a257f9580477720881022085426548aabfc151d7ebfe0ad7271044d145c1c76cef6aeebeb67d520ae3d1c`

- Expected authorization header:
  ```text
  Authorization: Ethereum_Secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000001,sig="0xe4b227b6cb3cbd30ac636b06f97b9e44488d966ca0d49a257f9580477720881022085426548aabfc151d7ebfe0ad7271044d145c1c76cef6aeebeb67d520ae3d1c"
  ```
