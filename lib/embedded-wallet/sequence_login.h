#include "sequence_wallet.h"
#include "requests/complete_auth_return.h"

int sequence_sign_in_with_email(const char *email);

SequenceCompleteAuthResponse sequence_confirm_email_sign_in(const char *email, const char *code);

sequence_wallet_t *sequence_use_wallet(const char *walletType);

sequence_wallet_t *sequence_create_wallet(const char *walletType);
