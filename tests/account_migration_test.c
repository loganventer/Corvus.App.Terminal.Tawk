/* Moving one account to another computer, end to end on two temporary
 * databases with the real archive and cipher: what travels and what does
 * not, the new account id, the search index, media paths, the label, the
 * expectation of the same number, a damaged or wrongly keyed package, and the
 * guard that unlinks the wrong number. */
#include "contracts/i_message_gateway.h"
#include "contracts/i_notifier.h"
#include "core/event.h"
#include "engines/table_catalog.h"
#include "infrastructure/authenticated_file_cipher.h"
#include "infrastructure/openssl_file_cipher.h"
#include "infrastructure/tar_archive.h"
#include "managers/account_export_manager.h"
#include "managers/account_import_manager.h"
#include "managers/account_link_guard.h"
#include "resource_access/sqlite_account_extractor.h"
#include "resource_access/sqlite_account_link_expectation.h"
#include "resource_access/sqlite_account_merger.h"
#include "resource_access/sqlite_account_store.h"
#include "resource_access/sqlite_database.h"
#include "resource_access/sqlite_schema_inspector.h"
#include "utilities/path_util.h"
#include "utilities/str_util.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

static void exec(sqlite3 *db, const char *sql) {
    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "sql failed: %s: %s\n", sql, err ? err : "?");
        sqlite3_free(err);
        failures++;
    }
}

static int scalar(sqlite3 *db, const char *sql) {
    sqlite3_stmt *st = NULL;
    int v = -1;
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) == SQLITE_OK && sqlite3_step(st) == SQLITE_ROW) v = sqlite3_column_int(st, 0);
    sqlite3_finalize(st);
    return v;
}

static void text_of(sqlite3 *db, const char *sql, char *out, size_t size) {
    sqlite3_stmt *st = NULL;
    out[0] = '\0';
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) == SQLITE_OK && sqlite3_step(st) == SQLITE_ROW) {
        str_copy(out, size, (const char *)sqlite3_column_text(st, 0));
    }
    sqlite3_finalize(st);
}

static void write_file(const char *path, const char *text) {
    FILE *f = fopen(path, "w");
    if (f) { fputs(text, f); fclose(f); }
}

/* ---- fakes for the link guard ------------------------------------------------ */

static int logouts, notices;
static int gw_logout(IMessageGateway *self) { (void)self; logouts++; return 0; }
static void fake_notify(INotifier *self, const Notification *n) { (void)self; (void)n; notices++; }

typedef struct Computer {
    char     dir[160], data_dir[200], db_path[240], media_dir[200], themes_dir[200], config_path[240], auth_dir[200], accounts_dir[200];
    sqlite3 *db;
} Computer;

static void make_computer(Computer *c, const char *root, const char *name) {
    snprintf(c->dir, sizeof(c->dir), "%s/%s", root, name);
    snprintf(c->data_dir, sizeof(c->data_dir), "%s/data", c->dir);
    snprintf(c->db_path, sizeof(c->db_path), "%s/tawk.db", c->data_dir);
    snprintf(c->media_dir, sizeof(c->media_dir), "%s/media", c->dir);
    snprintf(c->themes_dir, sizeof(c->themes_dir), "%s/themes", c->dir);
    snprintf(c->config_path, sizeof(c->config_path), "%s/config.ini", c->dir);
    snprintf(c->auth_dir, sizeof(c->auth_dir), "%s/auth", c->data_dir);
    snprintf(c->accounts_dir, sizeof(c->accounts_dir), "%s/accounts", c->data_dir);
    path_mkdir_p(c->data_dir, 0700);
    path_mkdir_p(c->media_dir, 0700);
    c->db = sqlite_database_open(c->db_path, NULL);
}

static BackupPaths paths_of(const Computer *c) {
    BackupPaths p = { c->data_dir, c->db_path, c->config_path, c->themes_dir, c->media_dir, c->auth_dir, c->accounts_dir };
    return p;
}

int main(void) {
    char root[] = "/tmp/tawk-migrate-XXXXXX";
    if (!mkdtemp(root)) return 1;
    IFileCipher *cipher = authenticated_file_cipher_create(openssl_file_cipher_create());
    IArchive *archive = tar_archive_create();
    if (!cipher || !archive || !cipher->available(cipher)) {
        printf("account migration: skipped (openssl or tar is missing)\n");
        return 0;
    }
    Computer old_pc, new_pc;
    make_computer(&old_pc, root, "old");
    make_computer(&new_pc, root, "new");
    if (!old_pc.db || !new_pc.db) { fprintf(stderr, "no database\n"); return 1; }

    /* The old computer: the first account, and "Personal" with its chats. */
    IAccountStore *old_accounts = sqlite_account_store_create(old_pc.db);
    AccountId personal = 0;
    old_accounts->add(old_accounts, "Personal", 3, 1790000000, &personal);
    old_accounts->set_identity(old_accounts, personal, "27609742113@s.whatsapp.net", "Logan");
    char pdf[300], sql[1500];
    snprintf(pdf, sizeof(pdf), "%s/outgoing/F1.pdf", old_pc.media_dir);
    char outgoing[300];
    snprintf(outgoing, sizeof(outgoing), "%s/outgoing", old_pc.media_dir);
    path_mkdir_p(outgoing, 0700);
    write_file(pdf, "%PDF-1.4 the report");
    snprintf(sql, sizeof(sql),
             "INSERT INTO chats (account_id, jid, name, last_ts) VALUES (%d, 'mom@s.whatsapp.net', 'Mom', 5), (1, 'main@s.whatsapp.net', 'Main chat', 5);"
             "INSERT INTO contacts (account_id, jid, name) VALUES (%d, 'mom@s.whatsapp.net', 'Mom');"
             "INSERT INTO messages (account_id, id, chat_jid, text, ts) VALUES (%d, 'M1', 'mom@s.whatsapp.net', 'hello there', 1), (1, 'M-main', 'main@s.whatsapp.net', 'private to main', 2);"
             "INSERT INTO messages (account_id, id, chat_jid, text, media_path, ts) VALUES (%d, 'M2', 'mom@s.whatsapp.net', 'the report', '%s', 3);"
             "INSERT INTO chat_prefs (jid, send_account) VALUES ('mom@s.whatsapp.net', %d);"
             "INSERT INTO chat_labels (jid, label) VALUES ('mom@s.whatsapp.net', 'family');"
             "INSERT INTO scheduled_messages (id, chat_jid, text, due_at, created_at, state, account_id) VALUES ('S1', 'mom@s.whatsapp.net', 'later', 99, 1, 0, %d);",
             personal, personal, personal, personal, pdf, personal, personal);
    exec(old_pc.db, sql);

    /* Nothing is left without a rule: a new table must be decided about before this passes. */
    ISchemaInspector *old_schema = sqlite_schema_inspector_create(old_pc.db);
    char tables[64][SCHEMA_TABLE_NAME_SIZE];
    int table_count = old_schema->tables(old_schema, NULL, tables, 64);
    const char *unknown = NULL;
    CHECK(table_count > 10 && table_catalog_unclassified((const char (*)[64])tables, table_count, &unknown) == 0,
          "every table in the database has a rule for moving an account");
    if (unknown) fprintf(stderr, "  no rule for table: %s\n", unknown);

    /* Export. */
    IAccountExtractor *extractor = sqlite_account_extractor_create(old_pc.db);
    AccountExportManagerDeps export_deps = { old_accounts, extractor, old_schema, archive, cipher };
    AccountExportManager *exporter = account_export_manager_create(&export_deps);
    Passphrase secret, wrong;
    passphrase_set(&secret, "correct horse battery");
    passphrase_set(&wrong, "not the passphrase");
    char package[300], why[600];
    snprintf(package, sizeof(package), "%s/personal.tawk", root);
    AccountExportRequest out = { paths_of(&old_pc), "Personal", package, 1 };
    MigrationReport report;
    int rc = account_export_manager_export(exporter, &out, &secret, &report, why, sizeof(why));
    if (rc != 0) fprintf(stderr, "  export said: %s\n", why);   /* a failure here makes the checks below fail too */
    CHECK(rc == 0, "an account is packed");
    CHECK(report.messages == 2 && report.chats == 1 && report.contacts == 1, "only that account's chats, messages and contacts are in it");
    CHECK(report.other_rows >= 2, "and the choices kept for its chats");
    CHECK(report.scheduled_left_behind == 1, "a message waiting for a time is counted as left behind");
    CHECK(report.media_files == 1, "and the file a message names comes with it");
    CHECK(account_export_manager_export(exporter, &out, &secret, &report, why, sizeof(why)) != 0, "an existing file is never replaced");
    AccountExportRequest unknown_account = { paths_of(&old_pc), "Nobody", "/tmp/never-written.tawk", 0 };
    CHECK(account_export_manager_export(exporter, &unknown_account, &secret, &report, why, sizeof(why)) != 0 &&
              access("/tmp/never-written.tawk", F_OK) != 0,
          "an account that does not exist writes nothing");

    /* The new computer already has an account called Personal. */
    IAccountStore *new_accounts = sqlite_account_store_create(new_pc.db);
    AccountId already = 0;
    new_accounts->add(new_accounts, "Personal", 1, 1790000100, &already);
    IAccountMerger *merger = sqlite_account_merger_create(new_pc.db);
    IAccountLinkExpectation *expectation = sqlite_account_link_expectation_create(new_pc.db);
    ISchemaInspector *new_schema = sqlite_schema_inspector_create(new_pc.db);
    IAccountExtractor *new_extractor = sqlite_account_extractor_create(new_pc.db);
    AccountImportManagerDeps import_deps = { new_accounts, merger, new_extractor, expectation, new_schema, archive, cipher };
    AccountImportManager *importer = account_import_manager_create(&import_deps);
    AccountImportRequest in = { paths_of(&new_pc), package, NULL };
    Account created;
    memset(&created, 0, sizeof(created));
    int accounts_before = scalar(new_pc.db, "SELECT COUNT(*) FROM accounts");

    CHECK(account_import_manager_import(importer, &in, &wrong, &report, &created, why, sizeof(why)) != 0, "the wrong passphrase brings nothing in");
    CHECK(scalar(new_pc.db, "SELECT COUNT(*) FROM accounts") == accounts_before, "and adds no account");

    /* A damaged package is refused before anything is decrypted. */
    char damaged[300];
    snprintf(damaged, sizeof(damaged), "%s/damaged.tawk", root);
    {
        FILE *f = fopen(package, "rb"), *g = fopen(damaged, "wb");
        int c, i = 0;
        while (f && g && (c = fgetc(f)) != EOF) { fputc(i == 200 ? (c ^ 0x55) : c, g); i++; }
        if (f) fclose(f);
        if (g) fclose(g);
    }
    AccountImportRequest damaged_in = { paths_of(&new_pc), damaged, NULL };
    CHECK(account_import_manager_import(importer, &damaged_in, &secret, &report, &created, why, sizeof(why)) != 0 &&
              scalar(new_pc.db, "SELECT COUNT(*) FROM accounts") == accounts_before,
          "a package changed after it was made is refused, and nothing is added");

    /* Import. */
    rc = account_import_manager_import(importer, &in, &secret, &report, &created, why, sizeof(why));
    CHECK(rc == 0, "the package is brought in");
    CHECK(created.id != already && created.id > 0, "as a new account, with an id of its own");
    CHECK(!strcmp(created.label, "Personal 2"), "its label gives way to the one already here");
    CHECK(created.agent_access == ACCOUNT_AGENT_OFF && !created.is_primary, "agents have no access to it, and it is not the primary account");
    CHECK(report.messages == 2 && report.chats == 1 && report.contacts == 1, "everything that was packed arrives");

    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM messages WHERE account_id = %d", created.id);
    CHECK(scalar(new_pc.db, sql) == 2, "the messages are the new account's");
    CHECK(scalar(new_pc.db, "SELECT COUNT(*) FROM messages WHERE id = 'M-main'") == 0, "another account's messages did not come");
    CHECK(scalar(new_pc.db, "SELECT COUNT(*) FROM chats WHERE jid = 'main@s.whatsapp.net'") == 0, "nor its chats");
    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM scheduled_messages WHERE account_id = %d", created.id);
    CHECK(scalar(new_pc.db, sql) == 0, "a message waiting for a time stayed behind");
    CHECK(scalar(new_pc.db, "SELECT COUNT(*) FROM chat_labels WHERE jid = 'mom@s.whatsapp.net' AND label = 'family'") == 1, "the labels of its chats came");
    snprintf(sql, sizeof(sql), "SELECT send_account FROM chat_prefs WHERE jid = 'mom@s.whatsapp.net'");
    CHECK(scalar(new_pc.db, sql) == created.id, "a chat that sent through this account still does, under its new id");
    if (scalar(new_pc.db, "SELECT COUNT(*) FROM sqlite_master WHERE name = 'messages_fts'") == 1) {
        CHECK(scalar(new_pc.db, "SELECT COUNT(*) FROM messages_fts WHERE messages_fts MATCH 'hello'") == 1, "the messages are searchable at once");
    }
    char moved[400], want[400];
    snprintf(sql, sizeof(sql), "SELECT media_path FROM messages WHERE account_id = %d AND id = 'M2'", created.id);
    text_of(new_pc.db, sql, moved, sizeof(moved));
    snprintf(want, sizeof(want), "%s/outgoing/F1.pdf", new_pc.media_dir);
    CHECK(!strcmp(moved, want) && path_is_regular_file(want), "a downloaded file is under the new media folder, and is there");
    char expected[128];
    expectation->expected(expectation, created.id, expected, sizeof(expected));
    CHECK(!strcmp(expected, "27609742113@s.whatsapp.net"), "the account waits for the number it came from");

    /* The same package again: a second, separate account, never mixed into the first. */
    int again = account_import_manager_import(importer, &in, &secret, &report, &created, why, sizeof(why));
    CHECK(again == 0 && !strcmp(created.label, "Personal 3"), "bringing it in twice makes a second account, not a mess");

    /* The guard: the wrong number is unlinked, the right one clears the wait. */
    IMessageGateway gateway;
    memset(&gateway, 0, sizeof(gateway));
    gateway.logout = gw_logout;
    INotifier notifier = { NULL, fake_notify, NULL };
    AccountLinkGuardDeps guard_deps = { expectation, &gateway, &notifier, created.id, created.label };
    AccountLinkGuard *guard = account_link_guard_create(&guard_deps);
    IEventObserver *observer = account_link_guard_observer(guard);
    Event e;
    event_init(&e, EVENT_AUTH_CONNECTED);
    str_copy(e.jid, sizeof(e.jid), "27829999999:4@s.whatsapp.net");
    observer->on_event(observer, &e);
    CHECK(logouts == 1 && notices == 1, "another number that links is unlinked again, and you are told");
    expectation->expected(expectation, created.id, expected, sizeof(expected));
    CHECK(expected[0] != '\0', "and the account still waits for the right one");
    str_copy(e.jid, sizeof(e.jid), "27609742113:7@s.whatsapp.net");
    observer->on_event(observer, &e);
    expectation->expected(expectation, created.id, expected, sizeof(expected));
    CHECK(logouts == 1 && expected[0] == '\0', "the right number links, and the wait is over");
    event_init(&e, EVENT_AUTH_CONNECTED);
    str_copy(e.jid, sizeof(e.jid), "27821111111@s.whatsapp.net");
    observer->on_event(observer, &e);
    CHECK(logouts == 1, "an account that was not moved is left alone");

    account_link_guard_destroy(guard);
    account_import_manager_destroy(importer);
    account_export_manager_destroy(exporter);
    sqlite3_close(old_pc.db);
    sqlite3_close(new_pc.db);
    char command[400];
    snprintf(command, sizeof(command), "rm -rf %s", root);
    if (system(command) != 0) fprintf(stderr, "note: could not remove %s\n", root);
    if (failures) { fprintf(stderr, "%d check(s) failed\n", failures); return 1; }
    printf("account migration: all checks passed\n");
    return 0;
}
