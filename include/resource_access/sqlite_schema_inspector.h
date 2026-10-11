#ifndef APP_RESOURCE_ACCESS_SQLITE_SCHEMA_INSPECTOR_H
#define APP_RESOURCE_ACCESS_SQLITE_SCHEMA_INSPECTOR_H

#include <sqlite3.h>

#include "contracts/i_schema_inspector.h"

/* `db` is the database tawk has open; other files are opened read only, as plain databases. */
ISchemaInspector *sqlite_schema_inspector_create(sqlite3 *db);

#endif
