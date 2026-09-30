#include "managers/database_crypt_manager.h"
#include "utilities/str_util.h"

#include <stdlib.h>
#include <unistd.h>

struct DatabaseCryptManager {
    DatabaseCryptManagerDeps deps;
};

DatabaseCryptManager *database_crypt_manager_create(const DatabaseCryptManagerDeps *deps) {
    DatabaseCryptManager *m = calloc(1, sizeof(*m));
    if (m) m->deps = *deps;
    return m;
}

void database_crypt_manager_destroy(DatabaseCryptManager *m) { free(m); }

static IDatabaseCipher *cipher(DatabaseCryptManager *m) { return m->deps.cipher; }

int database_crypt_manager_supported(DatabaseCryptManager *m) { return cipher(m)->supported(cipher(m)); }
int database_crypt_manager_is_encrypted(DatabaseCryptManager *m) { return cipher(m)->is_encrypted(cipher(m), m->deps.db_path); }

int database_crypt_manager_unlocks(DatabaseCryptManager *m, const Passphrase *key) {
    return cipher(m)->unlocks(cipher(m), m->deps.db_path, key);
}

static int refuse(char *why, unsigned long size, const char *text) {
    str_copy(why, size, text);
    return -1;
}

/* Common checks: the build can do it, and there is a database to change. */
static int ready(DatabaseCryptManager *m, char *why, unsigned long size) {
    if (!database_crypt_manager_supported(m)) return refuse(why, size, "this build of tawk has no SQLCipher, so it cannot encrypt");
    if (access(m->deps.db_path, F_OK) != 0) return refuse(why, size, "there is no database yet; start tawk once first");
    return 0;
}

int database_crypt_manager_encrypt(DatabaseCryptManager *m, const Passphrase *new_key, char *why, unsigned long size) {
    if (ready(m, why, size) != 0) return -1;
    if (database_crypt_manager_is_encrypted(m)) return refuse(why, size, "the database is already encrypted");
    return cipher(m)->reencrypt(cipher(m), m->deps.db_path, NULL, new_key, why, size);
}

int database_crypt_manager_decrypt(DatabaseCryptManager *m, const Passphrase *old_key, char *why, unsigned long size) {
    if (ready(m, why, size) != 0) return -1;
    if (!database_crypt_manager_is_encrypted(m)) return refuse(why, size, "the database is not encrypted");
    return cipher(m)->reencrypt(cipher(m), m->deps.db_path, old_key, NULL, why, size);
}

int database_crypt_manager_change(DatabaseCryptManager *m, const Passphrase *old_key, const Passphrase *new_key,
                                  char *why, unsigned long size) {
    if (ready(m, why, size) != 0) return -1;
    if (!database_crypt_manager_is_encrypted(m)) return refuse(why, size, "the database is not encrypted");
    return cipher(m)->reencrypt(cipher(m), m->deps.db_path, old_key, new_key, why, size);
}

int database_crypt_manager_plain_copies(DatabaseCryptManager *m, char out[][DATABASE_COPY_PATH_MAX], int max) {
    return cipher(m)->plain_copies(cipher(m), m->deps.db_path, out, max);
}

int database_crypt_manager_remove_copy(DatabaseCryptManager *m, const char *path) {
    return cipher(m)->shred(cipher(m), path);
}
