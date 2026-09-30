/* Encrypting your chats: a populated database is encrypted, opens only with
 * its passphrase, changes passphrase and is decrypted again with every row
 * intact; a rewrite that fails halfway leaves the original as it was; and
 * only one tawk at a time may use a data folder. */
#include "managers/database_crypt_manager.h"
#include "resource_access/sqlite_database.h"
#include "resource_access/sqlite_database_crypt.h"
#include "resource_access/sqlite_key.h"
#include "utilities/instance_lock.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

static int count(sqlite3 *db, const char *sql) {
    sqlite3_stmt *st = NULL;
    int v = -1;
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) == SQLITE_OK && sqlite3_step(st) == SQLITE_ROW) v = sqlite3_column_int(st, 0);
    sqlite3_finalize(st);
    return v;
}

/* A database as tawk leaves it, with some chats and messages. */
static void populate(const char *path) {
    sqlite3 *db = sqlite_database_open(path, NULL);
    if (!db) { failures++; return; }
    sqlite3_exec(db,
        "INSERT INTO chats (jid, name) VALUES ('27820000001@s.whatsapp.net', 'Mom'), ('120363000000000001@g.us', 'Dev team');"
        "INSERT INTO messages (id, chat_jid, sender_jid, text, ts, from_me) VALUES"
        " ('A1', '27820000001@s.whatsapp.net', '27820000001@s.whatsapp.net', 'Hello', 1790000000, 0),"
        " ('A2', '27820000001@s.whatsapp.net', 'me@s.whatsapp.net', 'Hi Mom', 1790000060, 1),"
        " ('A3', '120363000000000001@g.us', 'x@s.whatsapp.net', 'Standup at 9', 1790000120, 0);",
        NULL, NULL, NULL);
    sqlite_database_close(db);
}

static int rows_with(const char *path, const Passphrase *key) {
    sqlite3 *db = sqlite_database_open(path, key);
    if (!db) return -1;
    int n = count(db, "SELECT count(*) FROM messages") * 10 + count(db, "SELECT count(*) FROM chats");
    sqlite_database_close(db);
    return n;
}

static int contains_plain_text(const char *path, const char *needle) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    static char buf[1 << 20];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    return memmem(buf, n, needle, strlen(needle)) != NULL;
}

static void test_round_trip(const char *dir) {
    char path[600], blocker[700], copy[700];
    snprintf(path, sizeof(path), "%s/tawk.db", dir);
    snprintf(blocker, sizeof(blocker), "%s.rewriting", path);
    snprintf(copy, sizeof(copy), "%s.pre-v10", path);
    populate(path);
    FILE *f = fopen(copy, "w");                         /* a plain copy from before an upgrade */
    if (f) { fputs("SQLite format 3", f); fclose(f); }

    IDatabaseCipher *cipher = sqlite_database_cipher_create();
    DatabaseCryptManagerDeps deps = { cipher, path };
    DatabaseCryptManager *m = database_crypt_manager_create(&deps);
    Passphrase first, second, wrong;
    passphrase_set(&first, "correct horse battery");
    passphrase_set(&second, "a new passphrase");
    passphrase_set(&wrong, "not it");
    char why[256];

    CHECK(!database_crypt_manager_is_encrypted(m), "a new database is plain");
    mkdir(blocker, 0700);                               /* the new copy cannot be written */
    CHECK(database_crypt_manager_encrypt(m, &first, why, sizeof(why)) != 0, "a rewrite that cannot finish fails");
    CHECK(!database_crypt_manager_is_encrypted(m) && rows_with(path, NULL) == 32, "and leaves the original plain and whole");
    rmdir(blocker);

    CHECK(database_crypt_manager_encrypt(m, &first, why, sizeof(why)) == 0, "the database is encrypted");
    CHECK(database_crypt_manager_is_encrypted(m), "and is no longer plain SQLite");
    CHECK(!contains_plain_text(path, "Standup at 9"), "messages are not readable in the file");
    CHECK(database_crypt_manager_unlocks(m, &first) == 1 && database_crypt_manager_unlocks(m, &wrong) == 0,
          "only its passphrase opens it");
    CHECK(rows_with(path, &first) == 32, "every chat and message is there");
    CHECK(sqlite_database_open(path, &wrong) == NULL && sqlite_database_open(path, NULL) == NULL,
          "it cannot be opened without the passphrase");

    char copies[4][DATABASE_COPY_PATH_MAX];
    CHECK(database_crypt_manager_plain_copies(m, copies, 4) == 1 && strcmp(copies[0], copy) == 0,
          "a plain copy from before an upgrade is found");
    CHECK(database_crypt_manager_remove_copy(m, copies[0]) == 0 && access(copy, F_OK) != 0, "and removed");

    CHECK(database_crypt_manager_change(m, &wrong, &second, why, sizeof(why)) != 0, "a wrong current passphrase changes nothing");
    CHECK(database_crypt_manager_change(m, &first, &second, why, sizeof(why)) == 0, "the passphrase changes");
    CHECK(database_crypt_manager_unlocks(m, &second) == 1 && database_crypt_manager_unlocks(m, &first) == 0,
          "the new passphrase opens it and the old one no longer does");
    CHECK(rows_with(path, &second) == 32, "with everything still there");

    CHECK(database_crypt_manager_decrypt(m, &second, why, sizeof(why)) == 0, "the database is decrypted");
    CHECK(!database_crypt_manager_is_encrypted(m) && rows_with(path, NULL) == 32, "plain again, with every row");

    passphrase_wipe(&first);
    CHECK(first.length == 0 && first.text[0] == '\0', "a passphrase is wiped after use");
    database_crypt_manager_destroy(m);
    cipher->destroy(cipher);
}

static void test_instance_lock(const char *dir) {
    InstanceLock a, b;
    CHECK(instance_lock_acquire(&a, dir) == 0, "the first tawk takes the lock");
    CHECK(instance_lock_acquire(&b, dir) != 0, "a second one is refused");
    instance_lock_release(&a);
    CHECK(instance_lock_acquire(&b, dir) == 0, "and can take it once the first is gone");
    instance_lock_release(&b);
}

int main(void) {
    char dir[] = "/tmp/tawk-crypt-XXXXXX";
    if (!mkdtemp(dir)) { perror("mkdtemp"); return 1; }
    test_instance_lock(dir);
    if (sqlite_key_supported()) test_round_trip(dir);
    else printf("database_crypt_test: built without SQLCipher, encryption checks skipped\n");
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", dir);
    if (system(cmd) != 0) fprintf(stderr, "could not remove %s\n", dir);
    if (failures == 0) printf("ok: chats encrypt, open only with their passphrase, change it and decrypt again\n");
    return failures != 0;
}
