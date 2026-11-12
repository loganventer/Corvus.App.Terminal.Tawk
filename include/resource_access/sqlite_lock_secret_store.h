#ifndef APP_RESOURCE_ACCESS_SQLITE_LOCK_SECRET_STORE_H
#define APP_RESOURCE_ACCESS_SQLITE_LOCK_SECRET_STORE_H

#include <sqlite3.h>

#include "contracts/i_lock_secret_store.h"

/* The lock_secret table: one row, id 1. */
ILockSecretStore *sqlite_lock_secret_store_create(sqlite3 *db);

#endif
