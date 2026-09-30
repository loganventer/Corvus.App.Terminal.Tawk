#ifndef APP_RESOURCE_ACCESS_SQLITE_AUTOMATION_LOG_H
#define APP_RESOURCE_ACCESS_SQLITE_AUTOMATION_LOG_H

#include <sqlite3.h>

#include "contracts/i_automation_log.h"

/* The automation log in the automation_log table (migration 15), trimmed
 * to the newest few thousand entries. Borrows the connection. */
IAutomationLog *sqlite_automation_log_create(sqlite3 *db);

#endif
