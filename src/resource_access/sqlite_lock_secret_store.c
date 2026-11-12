#include "resource_access/sqlite_lock_secret_store.h"
#include "utilities/log.h"
#include "utilities/secure_zero.h"

#include <stdlib.h>
#include <string.h>

static sqlite3 *db_of(ILockSecretStore *self) { return (sqlite3 *)self->ctx; }

static int finish(ILockSecretStore *self, sqlite3_stmt *st) {
    int rc = sqlite3_step(st);
    if (rc != SQLITE_DONE) LOG_WARN("sqlite: %s", sqlite3_errmsg(db_of(self)));
    sqlite3_finalize(st);
    return rc == SQLITE_DONE ? 0 : -1;
}

static int secret_load(ILockSecretStore *self, LockVerifier *out) {
    lock_verifier_wipe(out);
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db_of(self), "SELECT salt, iterations, hash FROM lock_secret WHERE id = 1", -1, &st, NULL) != SQLITE_OK) return -1;
    int found = 0;
    if (sqlite3_step(st) == SQLITE_ROW &&
        sqlite3_column_bytes(st, 0) == LOCK_VERIFIER_SALT_BYTES && sqlite3_column_bytes(st, 2) == LOCK_VERIFIER_HASH_BYTES &&
        sqlite3_column_int64(st, 1) > 0) {
        memcpy(out->salt, sqlite3_column_blob(st, 0), LOCK_VERIFIER_SALT_BYTES);
        out->iterations = (uint32_t)sqlite3_column_int64(st, 1);
        memcpy(out->hash, sqlite3_column_blob(st, 2), LOCK_VERIFIER_HASH_BYTES);
        found = 1;
    }
    sqlite3_finalize(st);
    return found;
}

static int secret_save(ILockSecretStore *self, const LockVerifier *v) {
    if (!v || v->iterations == 0) return -1;
    sqlite3_stmt *st = NULL;
    static const char *const SQL = "INSERT INTO lock_secret (id, salt, iterations, hash) VALUES (1, ?, ?, ?) "
                                   "ON CONFLICT(id) DO UPDATE SET salt = excluded.salt, iterations = excluded.iterations, hash = excluded.hash";
    if (sqlite3_prepare_v2(db_of(self), SQL, -1, &st, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_blob(st, 1, v->salt, LOCK_VERIFIER_SALT_BYTES, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st, 2, v->iterations);
    sqlite3_bind_blob(st, 3, v->hash, LOCK_VERIFIER_HASH_BYTES, SQLITE_TRANSIENT);
    return finish(self, st);
}

static int secret_clear(ILockSecretStore *self) {
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db_of(self), "DELETE FROM lock_secret", -1, &st, NULL) != SQLITE_OK) return -1;
    return finish(self, st);
}

static void secret_destroy(ILockSecretStore *self) { free(self); }

ILockSecretStore *sqlite_lock_secret_store_create(sqlite3 *db) {
    ILockSecretStore *s = calloc(1, sizeof(*s));
    if (!s) return NULL;
    s->ctx = db;
    s->load = secret_load;
    s->save = secret_save;
    s->clear = secret_clear;
    s->destroy = secret_destroy;
    return s;
}
