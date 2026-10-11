#ifndef APP_CLIENTS_CLI_MIGRATE_COMMAND_H
#define APP_CLIENTS_CLI_MIGRATE_COMMAND_H

#include "contracts/i_database_snapshot.h"
#include "contracts/i_passphrase_prompt.h"
#include "core/account_export_request.h"
#include "core/account_import_request.h"
#include "managers/account_export_manager.h"
#include "managers/account_import_manager.h"

/* `tawk --migrate-out ACCOUNT FILE [--with-media]`: asks for a passphrase
 * twice and writes the encrypted package of one account. The caller holds
 * the instance lock. Returns the exit status. */
int migrate_out_command_run(AccountExportManager *mgr, IPassphrasePrompt *prompt, const AccountExportRequest *request);

/* `tawk --migrate-in FILE [--label NAME] [--yes]`: asks before it adds
 * anything (unless --yes), asks for the package's passphrase, brings the
 * account in and says what to do next. Returns the exit status. */
int migrate_in_command_run(AccountImportManager *mgr, IPassphrasePrompt *prompt, const AccountImportRequest *request, int assume_yes);

/* Before the database is opened for an import: keeps a copy of it as it is,
 * beside it, as tawk.db.before-migrate-DATE. `kept` gets the path. Returns 0
 * on success, and also when there is no database yet (nothing to keep). */
int migrate_keep_database_copy(IDatabaseSnapshot *snapshot, const char *data_dir, const char *db_path, char *kept, unsigned long size);

#endif
