#ifndef APP_MANAGERS_ACCOUNT_IMPORT_MANAGER_H
#define APP_MANAGERS_ACCOUNT_IMPORT_MANAGER_H

#include "core/account.h"
#include "core/account_import_request.h"
#include "core/migration_report.h"
#include "core/passphrase.h"
#include "managers/account_import_manager_deps.h"

/* Brings an account package from another computer in as a new account here,
 * beside the ones this tawk already has. Everything is checked before the
 * database is touched, the rows go in as one transaction, and the account
 * waits to be linked as the same number it came from. The caller holds the
 * instance lock. */
typedef struct AccountImportManager AccountImportManager;

AccountImportManager *account_import_manager_create(const AccountImportManagerDeps *deps);
void                  account_import_manager_destroy(AccountImportManager *mgr);

int account_import_manager_available(AccountImportManager *mgr);
/* Returns 0 and the new account in `created`, or -1 with the reason in `why`. */
int account_import_manager_import(AccountImportManager *mgr, const AccountImportRequest *request, const Passphrase *passphrase,
                                  MigrationReport *report, Account *created, char *why, unsigned long why_size);

#endif
