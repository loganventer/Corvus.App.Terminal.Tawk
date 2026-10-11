#ifndef APP_CONTRACTS_I_ACCOUNT_EXTRACTOR_H
#define APP_CONTRACTS_I_ACCOUNT_EXTRACTOR_H

#include "core/account_id.h"
#include "core/migration_report.h"

/* Takes one account's rows out of the database into a stand-alone file. */
typedef struct IAccountExtractor {
    void *ctx;
    /* Writes `package_db` (a new, plain database file) holding only the rows that belong to `account`, and
     * fills in what it carried. Returns 0 on success. */
    int  (*extract)(struct IAccountExtractor *self, AccountId account, const char *package_db, MigrationReport *report);
    /* Calls `visit` with every file path the packaged rows point to. Returns 0 on success. */
    int  (*media_paths)(struct IAccountExtractor *self, const char *package_db,
                        void (*visit)(void *visit_ctx, const char *path), void *visit_ctx);
    void (*destroy)(struct IAccountExtractor *self);
} IAccountExtractor;

#endif
