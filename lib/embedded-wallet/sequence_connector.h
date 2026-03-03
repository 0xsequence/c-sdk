#include "sequence_wallet.h"
#include "requests/complete_auth_return.h"

int sequence_sign_in_with_email(const char *email);

sequence_complete_auth_return sequence_confirm_email_sign_in(const char *email, const char *code);

sequence_wallet *sequence_use_wallet(const char *walletType);

sequence_wallet *sequence_create_wallet();

char *sequence_sign_message(const char *network, const char *message);

char *sequence_send_transaction(const char *network, const char *to, const char *value);
