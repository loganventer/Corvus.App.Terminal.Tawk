#ifndef APP_RESOURCE_ACCESS_SQLITE_DATABASE_H
#define APP_RESOURCE_ACCESS_SQLITE_DATABASE_H

#include <sqlite3.h>

#include "core/passphrase.h"

/* Opens (creating with mode 0600) and migrates the database. `key` is the
 * passphrase of an encrypted database, NULL for a plain one. Returns NULL
 * when it cannot be opened, including with a wrong passphrase. */
sqlite3 *sqlite_database_open(const char *path, const Passphrase *key);
void     sqlite_database_close(sqlite3 *db);

#endif
