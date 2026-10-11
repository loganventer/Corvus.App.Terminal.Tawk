#ifndef APP_CONTRACTS_I_ACCOUNT_MERGER_H
#define APP_CONTRACTS_I_ACCOUNT_MERGER_H

#include "core/merge_plan.h"
#include "core/migration_report.h"

/* Puts the rows of a package into the database, as one account's. */
typedef struct IAccountMerger {
    void *ctx;
    /* All of it or none of it: one transaction. Rows already here are kept as they are. Returns 0 on success. */
    int         (*merge)(struct IAccountMerger *self, const char *package_db, const MergePlan *plan, MigrationReport *report);
    /* Why the last merge failed, for people. */
    const char *(*error)(struct IAccountMerger *self);
    void        (*destroy)(struct IAccountMerger *self);
} IAccountMerger;

#endif
