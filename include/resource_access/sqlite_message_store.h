#ifndef APP_RESOURCE_ACCESS_SQLITE_MESSAGE_STORE_H
#define APP_RESOURCE_ACCESS_SQLITE_MESSAGE_STORE_H

#include <sqlite3.h>

#include "contracts/i_message_store.h"

IMessageStore *sqlite_message_store_create(sqlite3 *db);

#endif
