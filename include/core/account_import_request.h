#ifndef APP_CORE_ACCOUNT_IMPORT_REQUEST_H
#define APP_CORE_ACCOUNT_IMPORT_REQUEST_H

#include "core/backup_paths.h"

/* `tawk --migrate-in FILE [--label NAME]`. */
typedef struct AccountImportRequest {
    BackupPaths paths;
    const char *input;
    const char *label;          /* what to call it here; NULL keeps the name it had */
} AccountImportRequest;

#endif
