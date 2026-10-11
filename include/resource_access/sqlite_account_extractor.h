#ifndef APP_RESOURCE_ACCESS_SQLITE_ACCOUNT_EXTRACTOR_H
#define APP_RESOURCE_ACCESS_SQLITE_ACCOUNT_EXTRACTOR_H

#include <sqlite3.h>

#include "contracts/i_account_extractor.h"

IAccountExtractor *sqlite_account_extractor_create(sqlite3 *db);

#endif
