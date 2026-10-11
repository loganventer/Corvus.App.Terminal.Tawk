#ifndef APP_CORE_ACCOUNT_PACKAGE_MANIFEST_H
#define APP_CORE_ACCOUNT_PACKAGE_MANIFEST_H

#include <stdint.h>

#include "core/account.h"

#define ACCOUNT_PACKAGE_FORMAT 1

/* What an account package says about itself, written into it as account-manifest.txt. */
typedef struct AccountPackageManifest {
    int       format;                     /* ACCOUNT_PACKAGE_FORMAT when written by this version */
    char      app_version[32];
    int       schema_version;             /* the database's version when it was made */
    int64_t   created;                    /* epoch seconds */
    AccountId account_id;                 /* the account's id on the computer it came from */
    char      label[ACCOUNT_LABEL_SIZE];
    char      jid[128];                   /* the number it was linked as; "" when it never linked */
    char      name[128];
    int       colour;
    char      media_dir[512];             /* where its media was kept there */
    int       has_media;
} AccountPackageManifest;

#endif
