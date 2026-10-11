#ifndef APP_RESOURCE_ACCESS_SQLITE_ACCOUNT_MERGER_H
#define APP_RESOURCE_ACCESS_SQLITE_ACCOUNT_MERGER_H

#include <sqlite3.h>

#include "contracts/i_account_merger.h"

IAccountMerger *sqlite_account_merger_create(sqlite3 *db);

#endif
