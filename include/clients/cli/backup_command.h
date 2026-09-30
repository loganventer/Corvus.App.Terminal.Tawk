#ifndef APP_CLIENTS_CLI_BACKUP_COMMAND_H
#define APP_CLIENTS_CLI_BACKUP_COMMAND_H

#include "contracts/i_passphrase_prompt.h"
#include "core/backup_paths.h"
#include "core/backup_request.h"
#include "managers/backup_manager.h"

/* `tawk --backup FILE [--with-media] [--with-login]`: asks for a passphrase
 * twice and writes the encrypted backup. The caller holds the instance
 * lock. Returns the exit status. */
int backup_command_run(BackupManager *mgr, IPassphrasePrompt *prompt, const BackupRequest *request);

/* `tawk --restore FILE [--yes]`: asks before replacing anything (unless
 * --yes), asks for the backup's passphrase, restores and says what it did.
 * Returns the exit status. */
int restore_command_run(BackupManager *mgr, IPassphrasePrompt *prompt, const BackupPaths *paths, const char *input,
                        int assume_yes);

#endif
