#include "resource_access/sqlite_account_merger.h"
#include "engines/media_path_rewriter.h"
#include "engines/table_catalog.h"
#include "utilities/log.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Merger {
    sqlite3 *db;
    char     error[256];
} Merger;

static int fail(Merger *m, const char *what) {
    snprintf(m->error, sizeof(m->error), "%s: %s", what, sqlite3_errmsg(m->db));
    LOG_WARN("account import: %s", m->error);
    return -1;
}

static int exec(Merger *m, const char *sql) {
    char *err = NULL;
    if (sqlite3_exec(m->db, sql, NULL, NULL, &err) == SQLITE_OK) return 0;
    snprintf(m->error, sizeof(m->error), "%s", err ? err : "database error");
    LOG_WARN("account import: %s", m->error);
    sqlite3_free(err);
    return -1;
}

/* tawk_media_path(path): where a downloaded file is after the move, or '' when it is not coming. */
static void media_path_function(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    (void)argc;
    const MediaPathMap *map = sqlite3_user_data(ctx);
    const char *path = (const char *)sqlite3_value_text(argv[0]);
    char out[1100];
    if (path && media_path_rewrite(map, path, out, sizeof(out)) == 0) sqlite3_result_text(ctx, out, -1, SQLITE_TRANSIENT);
    else sqlite3_result_text(ctx, "", -1, SQLITE_STATIC);
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

static int has_column(sqlite3 *db, const char *schema, const char *table, const char *column) {
    char sql[200];
    snprintf(sql, sizeof(sql), "PRAGMA %s.table_info(%s)", schema, table);
    sqlite3_stmt *st = NULL;
    int found = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) != SQLITE_OK) return 0;
    while (!found && sqlite3_step(st) == SQLITE_ROW) found = strcmp((const char *)sqlite3_column_text(st, 1), column) == 0;
    sqlite3_finalize(st);
    return found;
}

/* One INSERT for `table`: the columns both sides have, each value taken from the package, with the account,
 * the media path and a chat's sending account translated. */
static int merge_table(Merger *m, const TableRule *rule, const MergePlan *plan, int *changed) {
    char sql[4096], cols[1500] = "", values[2500] = "";
    char col_sql[200];
    snprintf(col_sql, sizeof(col_sql), "PRAGMA pkg.table_info(%s)", rule->table);
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(m->db, col_sql, -1, &st, NULL) != SQLITE_OK) return fail(m, "reading the package");
    size_t cl = 0, vl = 0;
    while (sqlite3_step(st) == SQLITE_ROW) {
        const char *name = (const char *)sqlite3_column_text(st, 1);
        if (!has_column(m->db, "main", rule->table, name)) continue;      /* a column a newer tawk dropped */
        char value[200];
        if (!strcmp(name, "account_id")) snprintf(value, sizeof(value), "%d", plan->new_account);
        else if (!strcmp(name, "media_path")) snprintf(value, sizeof(value), "tawk_media_path(media_path)");
        else if (!strcmp(rule->table, "chat_prefs") && !strcmp(name, "send_account")) {
            snprintf(value, sizeof(value), "CASE WHEN send_account = %d THEN %d ELSE 0 END", plan->source_account, plan->new_account);
        } else snprintf(value, sizeof(value), "%s", name);
        cl += (size_t)snprintf(cols + cl, sizeof(cols) - cl, "%s%s", cl ? ", " : "", name);
        vl += (size_t)snprintf(values + vl, sizeof(values) - vl, "%s%s", vl ? ", " : "", value);
        if (cl >= sizeof(cols) - 64 || vl >= sizeof(values) - 256) break;
    }
    sqlite3_finalize(st);
    if (cl == 0) return 0;
    snprintf(sql, sizeof(sql), "INSERT OR IGNORE INTO main.%s (%s) SELECT %s FROM pkg.%s", rule->table, cols, values, rule->table);
    if (exec(m, sql) != 0) return -1;
    *changed = sqlite3_changes(m->db);
    return 0;
}

static int merge(IAccountMerger *self, const char *package_db, const MergePlan *plan, MigrationReport *report) {
    Merger *m = self->ctx;
    m->error[0] = '\0';
    memset(report, 0, sizeof(*report));
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(m->db, "ATTACH DATABASE ? AS pkg KEY ''", -1, &st, NULL) != SQLITE_OK) return fail(m, "opening the package");
    sqlite3_bind_text(st, 1, package_db, -1, SQLITE_STATIC);
    int attached = sqlite3_step(st) == SQLITE_DONE;
    sqlite3_finalize(st);
    if (!attached) return fail(m, "opening the package");

    MediaPathMap map = plan->media;
    int rc = sqlite3_create_function(m->db, "tawk_media_path", 1, SQLITE_UTF8, &map, media_path_function, NULL, NULL) == SQLITE_OK ? 0 : -1;
    if (rc == 0) rc = exec(m, "BEGIN IMMEDIATE");
    int in_transaction = rc == 0, n = 0;
    const TableRule *rules = table_catalog_rules(&n);
    for (int i = 0; rc == 0 && i < n; i++) {
        const TableRule *r = &rules[i];
        if (r->scope != TABLE_SCOPE_ACCOUNT && r->scope != TABLE_SCOPE_BY_JID) continue;
        if (!has_table(m->db, "pkg", r->table) || !has_table(m->db, "main", r->table)) continue;
        int changed = 0;
        rc = merge_table(m, r, plan, &changed);
        if (rc != 0) break;
        if (!strcmp(r->table, "messages")) report->messages = changed;
        else if (!strcmp(r->table, "chats")) report->chats = changed;
        else if (!strcmp(r->table, "contacts")) report->contacts = changed;
        else report->other_rows += changed;
    }
    if (in_transaction) {
        if (rc == 0) rc = exec(m, "COMMIT");
        if (rc != 0) sqlite3_exec(m->db, "ROLLBACK", NULL, NULL, NULL);
    }
    sqlite3_create_function(m->db, "tawk_media_path", 1, SQLITE_UTF8, NULL, NULL, NULL, NULL);
    sqlite3_exec(m->db, "DETACH DATABASE pkg", NULL, NULL, NULL);
    return rc;
}

static const char *merge_error(IAccountMerger *self) { return ((Merger *)self->ctx)->error; }

static void merger_destroy(IAccountMerger *self) {
    free(self->ctx);
    free(self);
}

IAccountMerger *sqlite_account_merger_create(sqlite3 *db) {
    IAccountMerger *a = calloc(1, sizeof(*a));
    Merger *m = calloc(1, sizeof(*m));
    if (!a || !m) { free(a); free(m); return NULL; }
    m->db = db;
    a->ctx = m;
    a->merge = merge;
    a->error = merge_error;
    a->destroy = merger_destroy;
    return a;
}
