#include "resource_access/sqlite_key.h"

int sqlite_key_supported(void) {
#ifdef APP_WITH_SQLCIPHER
    return 1;
#else
    return 0;
#endif
}

int sqlite_key_apply(sqlite3 *db, const Passphrase *key) {
#ifdef APP_WITH_SQLCIPHER
    /* SQLCipher reports a wrong passphrase on stderr, which would land on
     * the passphrase prompt; tawk says so itself. */
    sqlite3_exec(db, "PRAGMA cipher_log_level = NONE;", NULL, NULL, NULL);
#endif
    if (!key) return 0;
#ifdef APP_WITH_SQLCIPHER
    return sqlite3_key(db, key->text, (int)key->length) == SQLITE_OK ? 0 : -1;
#else
    (void)db;
    return -1;
#endif
}

int sqlite_key_verify(sqlite3 *db) {
    return sqlite3_exec(db, "SELECT count(*) FROM sqlite_master;", NULL, NULL, NULL) == SQLITE_OK ? 0 : -1;
}
