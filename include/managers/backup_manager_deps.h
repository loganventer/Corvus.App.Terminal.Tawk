#ifndef APP_MANAGERS_BACKUP_MANAGER_DEPS_H
#define APP_MANAGERS_BACKUP_MANAGER_DEPS_H

#include "contracts/i_archive.h"
#include "contracts/i_database_snapshot.h"
#include "contracts/i_file_cipher.h"

/* What the backup manager depends on, injected by the composition root. */
typedef struct BackupManagerDeps {
    IDatabaseSnapshot *snapshot;
    IArchive          *archive;
    IFileCipher       *cipher;
    int                database_encrypted;   /* tawk.db is encrypted (recorded in the manifest) */
} BackupManagerDeps;

#endif
