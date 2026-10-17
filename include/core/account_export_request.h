#ifndef APP_CORE_ACCOUNT_EXPORT_REQUEST_H
#define APP_CORE_ACCOUNT_EXPORT_REQUEST_H

#include "core/backup_paths.h"

/* `corvus-tawk --migrate-out ACCOUNT FILE [--with-media]`. */
typedef struct AccountExportRequest {
    BackupPaths paths;
    const char *account;        /* its label, or its id */
    const char *output;         /* must not exist yet */
    int         with_media;
} AccountExportRequest;

#endif
