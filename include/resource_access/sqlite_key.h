#ifndef APP_RESOURCE_ACCESS_SQLITE_KEY_H
#define APP_RESOURCE_ACCESS_SQLITE_KEY_H

#include <sqlite3.h>

#include "core/passphrase.h"

/* True when tawk was built with SQLCipher and can encrypt its database. */
int sqlite_key_supported(void);
/* Gives a just-opened connection its passphrase (before anything is read).
 * NULL means a plain database. Returns -1 when a passphrase is given but
 * this build cannot encrypt. */
int sqlite_key_apply(sqlite3 *db, const Passphrase *key);
/* Reads the schema, which fails with a wrong passphrase or a damaged file. */
int sqlite_key_verify(sqlite3 *db);

#endif
