# Sequence C SDK

### Setup

Get your access key from https://sequence.build/

```c
char *access_key = "oesk7yu5tjNfQElu5HjuUunAAAAAAAAAA";
sequence_config_init(access_key);
```

### Indexer

#### Get Token Balances

```c
uint64_t chain_id = 137;
char *contract_address = "0x3c499c542cEF5E3811e1192ce70d8cC03d5c3359";
char *wallet_address = "0x8e3E38fe7367dd3b52D1e281E4e8400447C8d8B9";

SequenceGetTokenBalancesReturn *tokenBalances = sequence_get_token_balances(
    chain_id,
    contract_address,
    wallet_address,
    true);

log_sequence_get_token_balances_return(tokenBalances);
free_sequence_token_balances_return(tokenBalances);
```

### Authentication

#### Sign in with Email

```c
char *email = "example@gmail.com";
sign_in_with_email(email);
```

#### Confirm Email Sign in

```c
char *email = "example@gmail.com";
char *code = "123456";

sequence_wallet_t *wallet = confirm_email_sign_in(email, code);
```

### Transactions

```c
uint64_t chain_id_mint = 80002;
char *contract_address_mint = "0x7e3DA4a3bC319962EF5CA37B05aD107Fc03cFBd6";
char *function_signature = "mint(address,uint256)";
char *address_arg = "0x7e3DA4a3bC319962EF5CA37B05aD107Fc03cFBd6";
char *uint256_arg = "123456789012345678901234567890123456789012345678901234567890";

Arg args[2];

args[0].type = ARG_STRING;
args[0].v.str = address_arg;

args[1].type = ARG_STRING;
args[1].v.str = uint256_arg;

char *hash = sequence_contract_call(
    wallet, // sequence_wallet_t from 'confirm_email_sign_in'
    chain_id_mint,
    contract_address_mint,
    0,
    function_signature,
    args, 2);
```
