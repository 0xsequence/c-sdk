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
   Authorization: ethereum-secp256k1 scope="{scope}",cred="{address}",nonce={nonce},sig="{signature}"
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
  - identityType: `email`
  - authMode: `otp`
  - handle: `test@example.com`

- Expected payload:
  ```json
  {"identityType":"email","authMode":"otp","metadata":{},"handle":"test@example.com"}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/CommitVerifier
  nonce: 1710000003

  {"identityType":"email","authMode":"otp","metadata":{},"handle":"test@example.com"}
  ```

- Expected digest hex:
  `0x033ecc5055e7181814097f54aa68fe7edbb5d3139064c5ae1c801ba1cfbecbcd`

- Expected signature:
  `0xab6db8389207099165cb01f7748b2451fefc54c29178d9cf71927d59c79c080906e21c50a04205fe7a5c27168377e06f924ae3ba1bcafe0208a1327789229f2d1b`

- Expected authorization header:
  ```text
  Authorization: ethereum-secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000003,sig="0xab6db8389207099165cb01f7748b2451fefc54c29178d9cf71927d59c79c080906e21c50a04205fe7a5c27168377e06f924ae3ba1bcafe0208a1327789229f2d1b"
  ```

## Vector: UseWallet

- Inputs:
  - endpoint: `/UseWallet`
  - nonce: `1710000004`
  - walletId: `wallet-123`

- Expected payload:
  ```json
  {"walletId":"wallet-123"}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/UseWallet
  nonce: 1710000004

  {"walletId":"wallet-123"}
  ```

- Expected digest hex:
  `0x2e5905f55ee7db83cb3354886c9ea47b3ffda34f979e70019b24a7effb5cecab`

- Expected signature:
  `0xc9607e16e7ad54ba8702c1f8977c1a2592561788e943a49841037c0b168f49d62b931ad5ec0d9c77a03749a0b0155406b1ab6ed7f8b8b65b19f8a059a774c3ca1c`

- Expected authorization header:
  ```text
  Authorization: ethereum-secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000004,sig="0xc9607e16e7ad54ba8702c1f8977c1a2592561788e943a49841037c0b168f49d62b931ad5ec0d9c77a03749a0b0155406b1ab6ed7f8b8b65b19f8a059a774c3ca1c"
  ```

## Vector: CreateWallet

- Inputs:
  - endpoint: `/CreateWallet`
  - nonce: `1710000005`
  - type: `ethereum`

- Expected payload:
  ```json
  {"type":"ethereum"}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/CreateWallet
  nonce: 1710000005

  {"type":"ethereum"}
  ```

- Expected digest hex:
  `0x1d8cb9d0c5397a5287afa89fc3c6530de756504602692e5f7d7cbdb3ae128e9e`

- Expected signature:
  `0x47ace7468b3084862e4434aefa50dc1b7ded03a32c9d8a8bcaad72f18612f72c6600924f8bafd5161c614f68295d0a0318b33819c494f86b3ab243909ec2378e1c`

- Expected authorization header:
  ```text
  Authorization: ethereum-secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000005,sig="0x47ace7468b3084862e4434aefa50dc1b7ded03a32c9d8a8bcaad72f18612f72c6600924f8bafd5161c614f68295d0a0318b33819c494f86b3ab243909ec2378e1c"
  ```

## Vector: SignMessage

- Inputs:
  - endpoint: `/SignMessage`
  - nonce: `1710000000`
  - walletId: `wallet-123`
  - network: `amoy`
  - message: `hello`

- Expected payload:
  ```json
  {"network":"amoy","walletId":"wallet-123","message":"hello"}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/SignMessage
  nonce: 1710000000

  {"network":"amoy","walletId":"wallet-123","message":"hello"}
  ```

- Expected digest hex:
  `0x2677991ae3d7458ca5a6332f9a97bd9be243c141588315249d2a3ee8ad63a5b5`

- Expected signature:
  `0xdd1846c9b0cf22224e3f4d48d69697cddf85d9cc0d3bc07e2000f056c7dd904f35bfee21ff04a8d1e2f21a914bc1599a2cd66a23e0f352046d811cbb7cd7bf0b1b`

- Expected authorization header:
  ```text
  Authorization: ethereum-secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000000,sig="0xdd1846c9b0cf22224e3f4d48d69697cddf85d9cc0d3bc07e2000f056c7dd904f35bfee21ff04a8d1e2f21a914bc1599a2cd66a23e0f352046d811cbb7cd7bf0b1b"
  ```

## Vector: SendTransaction

- Inputs:
  - endpoint: `/SendTransaction`
  - nonce: `1710000001`
  - walletId: `wallet-123`
  - network: `amoy`
  - to: `0xE5E8B483FfC05967FcFed58cc98D053265af6D99`
  - value: `1000`

- Expected payload:
  ```json
  {"network":"amoy","walletId":"wallet-123","to":"0xE5E8B483FfC05967FcFed58cc98D053265af6D99","value":"1000","mode":"relayer"}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/SendTransaction
  nonce: 1710000001

  {"network":"amoy","walletId":"wallet-123","to":"0xE5E8B483FfC05967FcFed58cc98D053265af6D99","value":"1000","mode":"relayer"}
  ```

- Expected digest hex:
  `0x0e6f40130b1d5f1f569d41e529062b5fc077beadae4c5abfc06ae62e258d6009`

- Expected signature:
  `0x929447ac2d417b51d0f6c18699871736e8e82e3e5c1e9f4e475ace34974127b16adfa4c611155be312f3ee05362095e7a17186ab97a4bdb55338ba33846aac6a1c`

- Expected authorization header:
  ```text
  Authorization: ethereum-secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000001,sig="0x929447ac2d417b51d0f6c18699871736e8e82e3e5c1e9f4e475ace34974127b16adfa4c611155be312f3ee05362095e7a17186ab97a4bdb55338ba33846aac6a1c"
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
  {"identityType":"email","authMode":"otp","verifier":"verifier-123","answer":"2oXiHHjzvN3XzdxGxWTK_c9hZf7pom0OovssPvI7q3M"}
  ```

- Expected preimage:
  ```text
  POST /rpc/Wallet/CompleteAuth
  nonce: 1710000002

  {"identityType":"email","authMode":"otp","verifier":"verifier-123","answer":"2oXiHHjzvN3XzdxGxWTK_c9hZf7pom0OovssPvI7q3M"}
  ```

- Expected digest hex:
  `0x74fcf06516ebe2767cf034bb4d9e73662cad443a243ab86e1e86bad94857270b`

- Expected signature:
  `0x925272704a36bdebf2643f884d5270cd5da1a344a0f55c569054757e680381254469b27ed9f816f4727dd5edc235a17ff1dd0c84f495f4dbd0fb291d499d806e1b`

- Expected authorization header:
  ```text
  Authorization: ethereum-secp256k1 scope="@1:test",cred="0x19e7e376e7c213b7e7e7e46cc70a5dd086daff2a",nonce=1710000002,sig="0x925272704a36bdebf2643f884d5270cd5da1a344a0f55c569054757e680381254469b27ed9f816f4727dd5edc235a17ff1dd0c84f495f4dbd0fb291d499d806e1b"
  ```

## Vector: CompleteAuth Answer Hash

- Inputs:
  - challenge: `challenge`
  - code: `123456`
  - verifier: `verifier-123`

- Expected answer hash:
  `2oXiHHjzvN3XzdxGxWTK_c9hZf7pom0OovssPvI7q3M`

- Expected payload:
  ```json
  {"identityType":"email","authMode":"otp","verifier":"verifier-123","answer":"2oXiHHjzvN3XzdxGxWTK_c9hZf7pom0OovssPvI7q3M"}
  ```
