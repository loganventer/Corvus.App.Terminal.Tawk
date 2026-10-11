#ifndef APP_MANAGERS_ACCOUNT_EXPORT_MANAGER_H
#define APP_MANAGERS_ACCOUNT_EXPORT_MANAGER_H

#include "core/account_export_request.h"
#include "core/migration_report.h"
#include "core/passphrase.h"
#include "managers/account_export_manager_deps.h"

/* Packs one account's chats, contacts, statuses and the choices kept for
 * its chats into an encrypted file that another computer's tawk can take in.
 * The WhatsApp login is never in it: the other computer links by QR code.
 * Nothing on this computer changes. The caller holds the instance lock. */
typedef struct AccountExportManager AccountExportManager;

AccountExportManager *account_export_manager_create(const AccountExportManagerDeps *deps);
void                  account_export_manager_destroy(AccountExportManager *mgr);

/* True when the encryption tool (openssl) is installed. */
int account_export_manager_available(AccountExportManager *mgr);
/* Returns 0 on success, or -1 with the reason in `why`. */
int account_export_manager_export(AccountExportManager *mgr, const AccountExportRequest *request,
                                  const Passphrase *passphrase, MigrationReport *report, char *why, unsigned long why_size);

#endif
