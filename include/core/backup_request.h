#ifndef APP_CORE_BACKUP_REQUEST_H
#define APP_CORE_BACKUP_REQUEST_H

#include "core/backup_paths.h"

/* `tawk --backup FILE [--with-media] [--with-login]`. */
typedef struct BackupRequest {
    BackupPaths paths;
    const char *output;          /* must not exist yet */
    int         with_media;
    int         with_login;
} BackupRequest;

#endif
