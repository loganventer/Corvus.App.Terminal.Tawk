#ifndef APP_MANAGERS_DATABASE_CRYPT_MANAGER_H
#define APP_MANAGERS_DATABASE_CRYPT_MANAGER_H

#include "contracts/i_database_cipher.h"
#include "core/passphrase.h"
#include "managers/database_crypt_manager_deps.h"

/* Encrypting your chats: turning encryption on and off, changing the
 * passphrase, and checking one at start-up. Every change is checked before
 * it replaces the database, and leaves the database as it was on failure. */
typedef struct DatabaseCryptManager DatabaseCryptManager;

DatabaseCryptManager *database_crypt_manager_create(const DatabaseCryptManagerDeps *deps);
void                  database_crypt_manager_destroy(DatabaseCryptManager *mgr);

/* This build can encrypt (it has SQLCipher). */
int  database_crypt_manager_supported(DatabaseCryptManager *mgr);
int  database_crypt_manager_is_encrypted(DatabaseCryptManager *mgr);
/* 1 when `key` opens the database, 0 when it does not, -1 on other errors. */
int  database_crypt_manager_unlocks(DatabaseCryptManager *mgr, const Passphrase *key);

/* Each returns 0 on success, or -1 with the reason in `why`. */
int  database_crypt_manager_encrypt(DatabaseCryptManager *mgr, const Passphrase *new_key, char *why, unsigned long size);
int  database_crypt_manager_decrypt(DatabaseCryptManager *mgr, const Passphrase *old_key, char *why, unsigned long size);
int  database_crypt_manager_change(DatabaseCryptManager *mgr, const Passphrase *old_key, const Passphrase *new_key,
                                   char *why, unsigned long size);

/* Plain copies kept beside the database before upgrades, which encrypting
 * would otherwise leave readable, and removing them (overwritten first). */
int  database_crypt_manager_plain_copies(DatabaseCryptManager *mgr, char out[][DATABASE_COPY_PATH_MAX], int max);
int  database_crypt_manager_remove_copy(DatabaseCryptManager *mgr, const char *path);

#endif
