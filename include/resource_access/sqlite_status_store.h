#ifndef APP_RESOURCE_ACCESS_SQLITE_STATUS_STORE_H
#define APP_RESOURCE_ACCESS_SQLITE_STATUS_STORE_H

#include <sqlite3.h>

#include "contracts/i_status_store.h"

IStatusStore *sqlite_status_store_create(sqlite3 *db);

#endif
