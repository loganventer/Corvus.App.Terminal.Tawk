#ifndef APP_RESOURCE_ACCESS_SQLITE_ACCOUNT_LINK_EXPECTATION_H
#define APP_RESOURCE_ACCESS_SQLITE_ACCOUNT_LINK_EXPECTATION_H

#include <sqlite3.h>

#include "contracts/i_account_link_expectation.h"

IAccountLinkExpectation *sqlite_account_link_expectation_create(sqlite3 *db);

#endif
