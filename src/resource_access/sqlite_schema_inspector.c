#include "resource_access/sqlite_schema_inspector.h"
#include "utilities/str_util.h"

#include <stdlib.h>

static sqlite3 *open_file(const char *path) {
    sqlite3 *db = NULL;
    if (sqlite3_open_v2(path, &db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        sqlite3_close(db);
        return NULL;
    }
    return db;
}

static int inspect_tables(ISchemaInspector *self, const char *file, char (*out)[SCHEMA_TABLE_NAME_SIZE], int max) {
    sqlite3 *db = file ? open_file(file) : (sqlite3 *)self->ctx;
    if (!db) return -1;
    sqlite3_stmt *st = NULL;
    int n = 0;
    if (sqlite3_prepare_v2(db, "SELECT name FROM sqlite_master WHERE type = 'table' ORDER BY name", -1, &st, NULL) != SQLITE_OK) {
        if (file) sqlite3_close(db);
        return -1;
    }
    while (n < max && sqlite3_step(st) == SQLITE_ROW) {
        str_copy(out[n++], SCHEMA_TABLE_NAME_SIZE, (const char *)sqlite3_column_text(st, 0));
    }
    sqlite3_finalize(st);
    if (file) sqlite3_close(db);
    return n;
}

static int inspect_version(ISchemaInspector *self, const char *file) {
    sqlite3 *db = file ? open_file(file) : (sqlite3 *)self->ctx;
    if (!db) return -1;
    sqlite3_stmt *st = NULL;
    int version = -1;
    if (sqlite3_prepare_v2(db, "PRAGMA user_version", -1, &st, NULL) == SQLITE_OK && sqlite3_step(st) == SQLITE_ROW) {
        version = sqlite3_column_int(st, 0);
    }
    sqlite3_finalize(st);
    if (file) sqlite3_close(db);
    return version;
}

static void inspect_destroy(ISchemaInspector *self) { free(self); }

ISchemaInspector *sqlite_schema_inspector_create(sqlite3 *db) {
    ISchemaInspector *s = calloc(1, sizeof(*s));
    if (!s) return NULL;
    s->ctx = db;
    s->tables = inspect_tables;
    s->version = inspect_version;
    s->destroy = inspect_destroy;
    return s;
}
