#ifndef APP_RESOURCE_ACCESS_SQLITE_SCHEDULED_MESSAGE_STORE_H
#define APP_RESOURCE_ACCESS_SQLITE_SCHEDULED_MESSAGE_STORE_H

#include <sqlite3.h>

#include "contracts/i_scheduled_message_store.h"

/* Scheduled messages in the scheduled_messages table (migration 12). Borrows the connection. */
IScheduledMessageStore *sqlite_scheduled_message_store_create(sqlite3 *db);

#endif
