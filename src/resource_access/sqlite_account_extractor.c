#include "resource_access/sqlite_account_extractor.h"
#include "engines/table_catalog.h"
#include "utilities/log.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static sqlite3 *db_of(IAccountExtractor *self) { return (sqlite3 *)self->ctx; }

static int run(sqlite3 *db, const char *sql) {
    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) == SQLITE_OK) return 0;
    LOG_WARN("account extraction: %s", err ? err : "?");
    sqlite3_free(err);
    return -1;
}

static int has_table(sqlite3 *db, const char *schema, const char *table) {
    char sql[160];
    snprintf(sql, sizeof(sql), "SELECT 1 FROM %s.sqlite_master WHERE type = 'table' AND name = ?", schema);
    sqlite3_stmt *st = NULL;
    int found = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) == SQLITE_OK) {
        sqlite3_bind_text(st, 1, table, -1, SQLITE_STATIC);
        found = sqlite3_step(st) == SQLITE_ROW;
    }
    sqlite3_finalize(st);
    return found;
}

static int count_rows(sqlite3 *db, const char *where_sql) {
    sqlite3_stmt *st = NULL;
    int n = 0;
    if (sqlite3_prepare_v2(db, where_sql, -1, &st, NULL) == SQLITE_OK && sqlite3_step(st) == SQLITE_ROW) n = sqlite3_column_int(st, 0);
    sqlite3_finalize(st);
    return n;
}

static int live_version(sqlite3 *db) { return count_rows(db, "PRAGMA main.user_version"); }

static int attach(sqlite3 *db, const char *package_db) {
    sqlite3_stmt *st = NULL;
    /* KEY '' makes the attached file a plain one even when the database it goes beside is encrypted. */
    if (sqlite3_prepare_v2(db, "ATTACH DATABASE ? AS pkg KEY ''", -1, &st, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(st, 1, package_db, -1, SQLITE_STATIC);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    return rc == SQLITE_DONE ? 0 : -1;
}

static int extract(IAccountExtractor *self, AccountId account, const char *package_db, MigrationReport *report) {
    sqlite3 *db = db_of(self);
    memset(report, 0, sizeof(*report));
    /* The connection may not create files, so the (private, empty) file is made here, and only if it is new. */
    unlink(package_db);
    int fd = open(package_db, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) return -1;
    close(fd);
    if (attach(db, package_db) != 0) { unlink(package_db); return -1; }
    int rc = 0, n = 0;
    const TableRule *rules = table_catalog_rules(&n);
    for (int i = 0; rc == 0 && i < n; i++) {
        const TableRule *r = &rules[i];
        if (!has_table(db, "main", r->table)) continue;
        char sql[512];
        if (r->scope == TABLE_SCOPE_ACCOUNT) {
            snprintf(sql, sizeof(sql), "CREATE TABLE pkg.%s AS SELECT * FROM main.%s WHERE account_id = %d", r->table, r->table, account);
        } else if (r->scope == TABLE_SCOPE_BY_JID) {
            snprintf(sql, sizeof(sql),
                     "CREATE TABLE pkg.%s AS SELECT * FROM main.%s WHERE %s IN (SELECT jid FROM main.chats WHERE account_id = %d)",
                     r->table, r->table, r->key_column, account);
        } else {
            continue;
        }
        rc = run(db, sql);
    }
    if (rc == 0) {
        char sql[96];
        snprintf(sql, sizeof(sql), "PRAGMA pkg.user_version = %d", live_version(db));
        rc = run(db, sql);
    }
    if (rc == 0) {
        report->messages = has_table(db, "pkg", "messages") ? count_rows(db, "SELECT COUNT(*) FROM pkg.messages") : 0;
        report->chats = has_table(db, "pkg", "chats") ? count_rows(db, "SELECT COUNT(*) FROM pkg.chats") : 0;
        report->contacts = has_table(db, "pkg", "contacts") ? count_rows(db, "SELECT COUNT(*) FROM pkg.contacts") : 0;
        for (int i = 0; i < n; i++) {
            const TableRule *r = &rules[i];
            if (r->scope == TABLE_SCOPE_LEFT_BEHIND && !strcmp(r->table, "scheduled_messages") && has_table(db, "main", r->table)) {
                char sql[160];
                snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM main.scheduled_messages WHERE account_id = %d AND state = 0", account);
                report->scheduled_left_behind = count_rows(db, sql);
            }
            if (r->scope != TABLE_SCOPE_ACCOUNT && r->scope != TABLE_SCOPE_BY_JID) continue;
            if (!strcmp(r->table, "messages") || !strcmp(r->table, "chats") || !strcmp(r->table, "contacts")) continue;
            if (!has_table(db, "pkg", r->table)) continue;
            char sql[96];
            snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM pkg.%s", r->table);
            report->other_rows += count_rows(db, sql);
        }
    }
    run(db, "DETACH DATABASE pkg");
    if (rc != 0) unlink(package_db);
    return rc;
}

static int media_paths(IAccountExtractor *self, const char *package_db,
                       void (*visit)(void *visit_ctx, const char *path), void *visit_ctx) {
    (void)self;
    sqlite3 *pkg = NULL;
    if (sqlite3_open_v2(package_db, &pkg, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        sqlite3_close(pkg);
        return -1;
    }
    const char *tables[] = { "messages", "statuses" };
    for (int t = 0; t < 2; t++) {
        char sql[160];
        snprintf(sql, sizeof(sql), "SELECT DISTINCT media_path FROM %s WHERE media_path <> ''", tables[t]);
        sqlite3_stmt *st = NULL;
        if (sqlite3_prepare_v2(pkg, sql, -1, &st, NULL) != SQLITE_OK) continue;   /* a table the package lacks */
        while (sqlite3_step(st) == SQLITE_ROW) visit(visit_ctx, (const char *)sqlite3_column_text(st, 0));
        sqlite3_finalize(st);
    }
    sqlite3_close(pkg);
    return 0;
}

static void extractor_destroy(IAccountExtractor *self) { free(self); }

IAccountExtractor *sqlite_account_extractor_create(sqlite3 *db) {
    IAccountExtractor *e = calloc(1, sizeof(*e));
    if (!e) return NULL;
    e->ctx = db;
    e->extract = extract;
    e->media_paths = media_paths;
    e->destroy = extractor_destroy;
    return e;
}
