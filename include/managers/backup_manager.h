#ifndef APP_MANAGERS_BACKUP_MANAGER_H
#define APP_MANAGERS_BACKUP_MANAGER_H

#include "core/backup_paths.h"
#include "core/backup_request.h"
#include "core/passphrase.h"
#include "core/restore_report.h"
#include "managers/backup_manager_deps.h"

/* Backups of your chats, always encrypted with a passphrase: the database,
 * settings and your own themes, and on request the media and the WhatsApp
 * login. Restoring checks everything in the file before it touches your
 * data, and moves what it replaces aside instead of deleting it. Neither
 * may run beside a running tawk (the caller holds the instance lock). */
typedef struct BackupManager BackupManager;

BackupManager *backup_manager_create(const BackupManagerDeps *deps);
void           backup_manager_destroy(BackupManager *mgr);

/* True when the encryption tool (openssl) is installed. */
int backup_manager_available(BackupManager *mgr);
/* Each returns 0 on success, or -1 with the reason in `why`. */
int backup_manager_backup(BackupManager *mgr, const BackupRequest *request, const Passphrase *passphrase,
                          char *why, unsigned long why_size);
int backup_manager_restore(BackupManager *mgr, const BackupPaths *paths, const char *input, const Passphrase *passphrase,
                           RestoreReport *report, char *why, unsigned long why_size);

#endif
