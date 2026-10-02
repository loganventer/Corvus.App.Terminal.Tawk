/* Upgrading an existing database: a v9 database (tawk 0.6) opened by this
 * version keeps every row, gains the new columns with their defaults, and
 * a copy of it as it was is kept beside it. A failed upgrade changes nothing. */
#include "resource_access/sqlite_database.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

static int scalar(sqlite3 *db, const char *sql) {
    sqlite3_stmt *st = NULL;
    int v = -1;
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) == SQLITE_OK && sqlite3_step(st) == SQLITE_ROW) v = sqlite3_column_int(st, 0);
    sqlite3_finalize(st);
    return v;
}

/* A database as tawk 0.6 left it: made now, then taken back to version 9. */
static void make_v9(const char *path, int keep_mentions_column) {
    sqlite3 *db = sqlite_database_open(path, NULL);
    if (!db) { failures++; return; }
    sqlite3_exec(db,
        "ALTER TABLE messages DROP COLUMN quoted_status;"
        "ALTER TABLE messages DROP COLUMN link_desc; ALTER TABLE messages DROP COLUMN link_title;"
        "ALTER TABLE messages DROP COLUMN link_url; DROP TABLE scheduled_messages;"
        "ALTER TABLE messages DROP COLUMN forwarded; ALTER TABLE chats DROP COLUMN unread_mention;"
        "ALTER TABLE messages DROP COLUMN mentions_me;", NULL, NULL, NULL);
    if (!keep_mentions_column) sqlite3_exec(db, "ALTER TABLE messages DROP COLUMN mentions;", NULL, NULL, NULL);
    sqlite3_exec(db,
        "PRAGMA user_version = 9;"
        "INSERT INTO chats (jid, name) VALUES ('27820000001@s.whatsapp.net', 'Mom');"
        "INSERT INTO messages (id, chat_jid, sender_jid, text, ts, from_me) VALUES"
        " ('A1', '27820000001@s.whatsapp.net', '27820000001@s.whatsapp.net', 'Hello *there*', 1790000000, 0),"
        " ('A2', '27820000001@s.whatsapp.net', 'me@s.whatsapp.net', 'Hi Mom', 1790000060, 1);",
        NULL, NULL, NULL);
    sqlite_database_close(db);
}

static void test_upgrade(const char *dir) {
    char path[600], copy[700];
    snprintf(path, sizeof(path), "%s/tawk.db", dir);
    snprintf(copy, sizeof(copy), "%s.pre-v10", path);
    make_v9(path, 0);

    sqlite3 *db = sqlite_database_open(path, NULL);
    CHECK(db != NULL, "a v9 database opens and upgrades");
    if (!db) return;
    CHECK(scalar(db, "PRAGMA user_version") == 16, "it is now version 16");
    CHECK(scalar(db, "SELECT count(*) FROM messages") == 2 && scalar(db, "SELECT count(*) FROM chats") == 1, "every row survives");
    CHECK(scalar(db, "SELECT count(*) FROM messages WHERE mentions IS NULL AND mentions_me = 0 AND forwarded = 0 AND link_url IS NULL") == 2,
          "old messages get the new columns with their defaults");
    CHECK(scalar(db, "SELECT unread_mention FROM chats") == 0, "chats get the new column too");
    CHECK(scalar(db, "SELECT count(*) FROM scheduled_messages") == 0, "the scheduled messages table exists");
    CHECK(scalar(db, "SELECT count(*) FROM automation_log") == 0, "the automation log table exists");
    CHECK(scalar(db, "SELECT count(*) FROM messages WHERE text = 'Hello *there*'") == 1, "text is kept as it was");
    sqlite_database_close(db);

    sqlite3 *old = NULL;
    CHECK(access(copy, F_OK) == 0 && sqlite3_open_v2(copy, &old, SQLITE_OPEN_READONLY, NULL) == SQLITE_OK &&
          scalar(old, "PRAGMA user_version") == 9 && scalar(old, "SELECT count(*) FROM messages") == 2,
          "a copy of the v9 database is kept beside it");
    /* tawk 0.6 reads only the columns it knows; they are all still there. */
    CHECK(scalar(old, "SELECT count(*) FROM messages") == 2, "the copy opens as tawk 0.6 left it");
    sqlite3_close(old);

    db = sqlite_database_open(path, NULL);
    CHECK(db != NULL, "the upgraded database opens again");
    sqlite_database_close(db);
    sqlite3 *v9reader = NULL;
    sqlite3_open_v2(path, &v9reader, SQLITE_OPEN_READONLY, NULL);
    CHECK(scalar(v9reader, "SELECT count(*) FROM (SELECT id, chat_jid, sender_jid, sender_name, text, media_ref, media_path, type,"
                           " status, ts, from_me, duration, quoted_id, quoted_sender, quoted_text, thumbnail, edited, deleted FROM messages)") == 2,
          "tawk 0.6's column list still reads the upgraded database");
    sqlite3_close(v9reader);
    unlink(path);
    unlink(copy);
}

static void test_failed_upgrade(const char *dir) {
    char path[600], copy[700];
    snprintf(path, sizeof(path), "%s/broken.db", dir);
    snprintf(copy, sizeof(copy), "%s.pre-v10", path);
    make_v9(path, 1);                                  /* the mentions column already exists: version 10 fails */
    sqlite3 *db = sqlite_database_open(path, NULL);
    CHECK(db == NULL, "a failing upgrade refuses to open the database");
    sqlite3 *raw = NULL;
    sqlite3_open_v2(path, &raw, SQLITE_OPEN_READONLY, NULL);
    CHECK(scalar(raw, "PRAGMA user_version") == 9 && scalar(raw, "SELECT count(*) FROM messages") == 2,
          "and leaves it at version 9 with every row");
    sqlite3_close(raw);
    CHECK(access(copy, F_OK) == 0, "the copy from before the upgrade is there to fall back on");
    unlink(path);
    unlink(copy);
}

int main(void) {
    char dir[] = "/tmp/tawk-migration-XXXXXX";
    if (!mkdtemp(dir)) { perror("mkdtemp"); return 1; }
    test_upgrade(dir);
    test_failed_upgrade(dir);
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", dir);
    if (system(cmd) != 0) fprintf(stderr, "could not remove %s\n", dir);
    if (failures == 0) printf("ok: a 0.6 database upgrades, keeps its rows and a copy, and a failed upgrade changes nothing\n");
    return failures != 0;
}
