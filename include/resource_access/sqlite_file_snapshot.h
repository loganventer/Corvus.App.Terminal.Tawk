#ifndef APP_RESOURCE_ACCESS_SQLITE_FILE_SNAPSHOT_H
#define APP_RESOURCE_ACCESS_SQLITE_FILE_SNAPSHOT_H

#include "contracts/i_database_snapshot.h"

/* Copies the database file and its write-ahead log as they are. That is a
 * consistent snapshot because nothing has the database open while the
 * instance lock is held, and it needs no passphrase: an encrypted database
 * is copied encrypted. */
IDatabaseSnapshot *sqlite_file_snapshot_create(void);

#endif
