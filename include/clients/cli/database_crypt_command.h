#ifndef APP_CLIENTS_CLI_DATABASE_CRYPT_COMMAND_H
#define APP_CLIENTS_CLI_DATABASE_CRYPT_COMMAND_H

#include "clients/cli/database_crypt_action.h"
#include "contracts/i_passphrase_prompt.h"
#include "managers/database_crypt_manager.h"

/* `tawk --encrypt`, `--decrypt` and `--change-passphrase`. The caller
 * holds the instance lock. Returns the exit status: 0 done, 1 failed or
 * refused. */
int database_crypt_command_run(DatabaseCryptAction action, DatabaseCryptManager *mgr, IPassphrasePrompt *prompt,
                               int assume_yes);

/* At start-up: when the database is encrypted, asks for its passphrase
 * (three tries) and leaves it in `out`. Returns 0 for a plain database
 * (nothing asked), 1 when `out` holds the right passphrase, -1 when it
 * could not be unlocked (a message has been printed). */
int database_unlock(DatabaseCryptManager *mgr, IPassphrasePrompt *prompt, Passphrase *out);

#endif
