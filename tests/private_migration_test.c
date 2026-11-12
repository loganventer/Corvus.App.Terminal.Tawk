/* Version 28: a soft-locked chat becomes a private one, the lock password
 * gets its table, and a copy of the database as it was is kept. */
#include "resource_access/sqlite_database.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures;
#define CHECK(cond, what) do { if (!(cond)) { printf("FAIL: %s\n", what); failures++; } } while (0)

static int scalar(sqlite3 *db, const char *sql) {
    sqlite3_stmt *st = NULL;
    int v = -1;
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) == SQLITE_OK && sqlite3_step(st) == SQLITE_ROW) v = sqlite3_column_int(st, 0);
    sqlite3_finalize(st);
    return v;
}

static void run(sqlite3 *db, const char *sql, const char *what) {
    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) { printf("FAIL: %s: %s\n", what, err ? err : "?"); failures++; }
    sqlite3_free(err);
}

/* Version 27 differs from 28 only in the column's name and the missing
 * table, so it is made by turning a new database back. */
static void make_v27(const char *path) {
    sqlite3 *db = sqlite_database_open(path, NULL);
    if (!db) { printf("FAIL: a new database opens\n"); failures++; return; }
    run(db, "ALTER TABLE chats RENAME COLUMN is_private TO soft_locked;"
            "DROP TABLE lock_secret;"
            "INSERT INTO chats (account_id, jid, name, soft_locked) VALUES (1, 'a@s.whatsapp.net', 'Hidden', 1);"
            "INSERT INTO chats (account_id, jid, name, soft_locked) VALUES (1, 'b@s.whatsapp.net', 'Open', 0);"
            "PRAGMA user_version = 27;", "the v27 database");
    sqlite_database_close(db);
}

int main(void) {
    char dir[] = "/tmp/tawk-private-migration-XXXXXX";
    if (!mkdtemp(dir)) { perror("mkdtemp"); return 1; }
    char path[600], copy[700];
    snprintf(path, sizeof(path), "%s/tawk.db", dir);
    snprintf(copy, sizeof(copy), "%s.pre-v28", path);

    make_v27(path);
    sqlite3 *db = sqlite_database_open(path, NULL);
    CHECK(db != NULL, "a v27 database opens");
    if (db) {
        CHECK(scalar(db, "PRAGMA user_version") == 28, "it is now version 28");
        CHECK(scalar(db, "SELECT is_private FROM chats WHERE jid = 'a@s.whatsapp.net'") == 1, "the soft-locked chat is private");
        CHECK(scalar(db, "SELECT is_private FROM chats WHERE jid = 'b@s.whatsapp.net'") == 0, "the other chat is not");
        CHECK(scalar(db, "SELECT count(*) FROM pragma_table_info('chats') WHERE name = 'soft_locked'") == 0, "the old column is gone");
        CHECK(scalar(db, "SELECT count(*) FROM lock_secret") == 0, "the lock password table is there and empty");
        CHECK(sqlite3_exec(db, "INSERT INTO lock_secret (id, salt, iterations, hash) VALUES (2, x'00', 1, x'00')", NULL, NULL, NULL) != SQLITE_OK,
              "and holds only one row");
        sqlite_database_close(db);
    }
    sqlite3 *old = NULL;
    CHECK(access(copy, F_OK) == 0 && sqlite3_open_v2(copy, &old, SQLITE_OPEN_READONLY, NULL) == SQLITE_OK &&
          scalar(old, "PRAGMA user_version") == 27 && scalar(old, "SELECT soft_locked FROM chats WHERE jid = 'a@s.whatsapp.net'") == 1,
          "a copy of the v27 database is kept as it was");
    sqlite3_close(old);

    char cmd[128];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", dir);
    if (system(cmd) != 0) printf("could not remove %s\n", dir);
    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: soft-locked chats become private, and the lock password has its table\n");
    return 0;
}
