#ifndef APP_CONTRACTS_I_DATABASE_CIPHER_H
#define APP_CONTRACTS_I_DATABASE_CIPHER_H

#include "core/passphrase.h"

#define DATABASE_COPY_PATH_MAX 1100

/* Encryption of the database file: whether it is encrypted, whether a
 * passphrase opens it, rewriting it under another passphrase, and removing
 * plain copies of it (kept before upgrades) that would defeat encrypting it. */
typedef struct IDatabaseCipher {
    void *ctx;
    int  (*supported)(struct IDatabaseCipher *self);
    int  (*is_encrypted)(struct IDatabaseCipher *self, const char *db_path);
    /* 1 when `key` opens it, 0 when it does not, -1 on other errors. */
    int  (*unlocks)(struct IDatabaseCipher *self, const char *db_path, const Passphrase *key);
    /* NULL keys mean plain. Leaves the original untouched on failure. */
    int  (*reencrypt)(struct IDatabaseCipher *self, const char *db_path, const Passphrase *old_key,
                      const Passphrase *new_key, char *why, unsigned long why_size);
    /* Unencrypted copies beside the database (tawk.db.pre-v10 and the like). */
    int  (*plain_copies)(struct IDatabaseCipher *self, const char *db_path, char out[][DATABASE_COPY_PATH_MAX], int max);
    /* Overwrites a copy and removes it. */
    int  (*shred)(struct IDatabaseCipher *self, const char *path);
    void (*destroy)(struct IDatabaseCipher *self);
} IDatabaseCipher;

#endif
