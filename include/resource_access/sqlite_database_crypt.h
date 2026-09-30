#ifndef APP_RESOURCE_ACCESS_SQLITE_DATABASE_CRYPT_H
#define APP_RESOURCE_ACCESS_SQLITE_DATABASE_CRYPT_H

#include "contracts/i_database_cipher.h"
#include "core/passphrase.h"

/* Encrypting, decrypting and re-keying tawk.db with SQLCipher. None of
 * these may run while tawk has the database open (take the instance lock). */

/* True when the file exists and is not a plain SQLite database (it starts
 * with random bytes instead of "SQLite format 3"). */
int sqlite_database_is_encrypted(const char *path);
/* 1 when `key` opens the database, 0 when it does not, -1 on other errors. */
int sqlite_database_check_key(const char *path, const Passphrase *key);
/* Rewrites the database under `new_key` (NULL: plain) from `old_key`
 * (NULL: plain). The new copy is written beside it, checked table by table
 * against the original, then put in its place; a plain original is
 * overwritten before it is removed. On any failure the original is left as
 * it was. Returns 0 on success; `why` says what went wrong otherwise. */
int sqlite_database_reencrypt(const char *path, const Passphrase *old_key, const Passphrase *new_key,
                              char *why, unsigned long why_size);

/* The functions above behind IDatabaseCipher. */
IDatabaseCipher *sqlite_database_cipher_create(void);

#endif
