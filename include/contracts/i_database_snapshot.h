#ifndef APP_CONTRACTS_I_DATABASE_SNAPSHOT_H
#define APP_CONTRACTS_I_DATABASE_SNAPSHOT_H

/* A consistent copy of the database for a backup, taken while no tawk has
 * it open (the caller holds the instance lock). An encrypted database stays
 * encrypted in the copy. */
typedef struct IDatabaseSnapshot {
    void *ctx;
    /* Copies `db_path` (and its write-ahead log, if any) into `dest_dir` as
     * tawk.db. Returns 0 on success. */
    int  (*take)(struct IDatabaseSnapshot *self, const char *db_path, const char *dest_dir);
    void (*destroy)(struct IDatabaseSnapshot *self);
} IDatabaseSnapshot;

#endif
