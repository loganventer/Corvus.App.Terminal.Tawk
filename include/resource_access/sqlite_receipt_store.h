#ifndef APP_RESOURCE_ACCESS_SQLITE_RECEIPT_STORE_H
#define APP_RESOURCE_ACCESS_SQLITE_RECEIPT_STORE_H

#include <sqlite3.h>

#include "contracts/i_receipt_store.h"

IReceiptStore *sqlite_receipt_store_create(sqlite3 *db);

#endif
