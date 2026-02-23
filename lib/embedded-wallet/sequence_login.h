#include "sequence_wallet.h"
#include "requests/complete_auth_return.h"

int sign_in_with_email(const char *email);

SequenceCompleteAuthResponse confirm_email_sign_in(const char *email, const char *code);

int use_wallet(const char *walletType);

int create_wallet(const char *walletType);
