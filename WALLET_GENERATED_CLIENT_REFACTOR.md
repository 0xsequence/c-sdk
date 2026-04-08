# Wallet Generated Client Refactor

## Goal

Keep the WebRPC C generator simple and move `c-sdk` toward a thin Sequence-specific layer built on top of the generated WAAS client.

## Principles

- Use generated WAAS request/response models directly wherever practical.
- Keep the generator limited to:
  - generated models
  - generated one-shot client methods
  - generated `prepare_request`
  - generated `send_prepared_request`
  - generated `parse_response`
- Keep Sequence-specific wrapper logic only for:
  - signed wallet request headers
  - access key handling
  - secure storage/session state
  - convenience conversions like `chain_id -> network`

## What Should Not Come Back

- No handwritten WAAS request JSON builders.
- No handwritten WAAS response model mirrors.
- No WAAS-specific generator behavior for `c-sdk`.
- No second local model system for wallet/auth flows.

## Current Direction

- The public wallet/auth API now uses generated WAAS models directly.
- Legacy compatibility aliases have been removed from the wallet/auth surface.
- The remaining wrapper cost is mostly Sequence-specific behavior, not transport/model duplication.

## Architectural Target

`c-sdk` should not behave like a second hand-maintained WAAS client. It should be a thin Sequence-specific layer on top of the generated WAAS client.

That means:

- Generated WAAS models are the default model language for wallet/auth flows.
- Generated WAAS transport primitives are the default request/response path.
- The SDK-owned logic should be limited to:
  - signed wallet request headers
  - access key handling
  - secure storage/session lifecycle
  - small convenience conversions such as `chain_id -> network`

The goal is not just "lowest risk". The goal is the correct long-term layering with the generator kept simple.

## Public API Ownership Rule

For public wallet/auth functions that return generated WAAS models:

- the returned object is heap-allocated
- the caller owns it
- it must be released with the matching generated `waas_*_free(...)` function plus `free(...)`

This ownership rule should be explicit before further structural changes.

## Next Structural Improvements

1. Lock the public wallet/auth contract around generated WAAS models and explicit ownership.
2. Split wallet code by responsibility instead of keeping a single large `sequence_connector.c`.
3. Keep a shared internal wallet client layer for:
   - signer/session globals
   - request signing
   - prepared request sending
   - common RPC execution helpers
4. Separate auth/session flows from wallet operation flows.
5. Keep the advanced path explicit: generated client usage should remain directly usable for callers that want lower-level control.
6. Keep the wallet/auth public surface clean: no stale compatibility aliases or duplicate local wallet/auth model headers.

## Desired File Shape

- `lib/wallet/sequence_wallet_shared.c`
  - shared Sequence-specific WAAS client glue
- `lib/wallet/sequence_auth.c`
  - sign-in / confirm-auth / restore-session
- `lib/wallet/sequence_wallet_ops.c`
  - use-wallet / create-wallet / sign-message / send-transaction
- `lib/wallet/sequence_wallet_internal.h`
  - internal shared helper contracts only
- `lib/wallet/sequence_connector.h`
  - public API surface only

## Expected Benefits

- Smaller conceptual surface per file
- Clearer separation between generated client usage and SDK-specific behavior
- Less repeated typed setup/cleanup code concentrated in one monolith
- Easier maintenance around signing/session logic
- A wallet/auth layer that is properly built on generated WAAS rather than informally shadowing it
