#ifndef APP_RESOURCE_ACCESS_SQLITE_REACTION_STORE_H
#define APP_RESOURCE_ACCESS_SQLITE_REACTION_STORE_H

#include <sqlite3.h>

#include "contracts/i_reaction_store.h"

IReactionStore *sqlite_reaction_store_create(sqlite3 *db);

#endif
