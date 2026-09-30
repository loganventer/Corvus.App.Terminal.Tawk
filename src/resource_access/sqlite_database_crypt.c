#include "resource_access/sqlite_database_crypt.h"
#include "resource_access/sqlite_key.h"
#include "utilities/log.h"
#include "utilities/str_util.h"

#include <dirent.h>
#include <fcntl.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define PLAIN_HEADER "SQLite format 3"

int sqlite_database_is_encrypted(const char *path) {
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return 0;
    char head[16];
    ssize_t n = read(fd, head, sizeof(head));
    close(fd);
    return n == (ssize_t)sizeof(head) && memcmp(head, PLAIN_HEADER, sizeof(PLAIN_HEADER) - 1) != 0;
}

static sqlite3 *open_with(const char *path, const Passphrase *key) {
    sqlite3 *db = NULL;
    if (sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK ||
        sqlite_key_apply(db, key) != 0 || sqlite_key_verify(db) != 0) {
        sqlite3_close(db);
        return NULL;
    }
    return db;
}

int sqlite_database_check_key(const char *path, const Passphrase *key) {
    if (access(path, F_OK) != 0) return -1;
    sqlite3 *db = open_with(path, key);
    if (!db) return 0;
    sqlite3_close(db);
    return 1;
}

static int scalar(sqlite3 *db, const char *sql, long long *out) {
    sqlite3_stmt *st = NULL;
    int ok = sqlite3_prepare_v2(db, sql, -1, &st, NULL) == SQLITE_OK && sqlite3_step(st) == SQLITE_ROW;
    if (ok) *out = sqlite3_column_int64(st, 0);
    sqlite3_finalize(st);
    return ok ? 0 : -1;
}

/* The copy holds every table of the original with the same number of rows,
 * and the same schema version. */
static int same_contents(sqlite3 *src, sqlite3 *copy) {
    long long a = 0, b = -1;
    if (scalar(src, "PRAGMA user_version", &a) != 0 || scalar(copy, "PRAGMA user_version", &b) != 0 || a != b) return 0;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(src, "SELECT name FROM sqlite_master WHERE type = 'table' AND name NOT LIKE 'sqlite_%'",
                           -1, &st, NULL) != SQLITE_OK) return 0;
    int same = 1;
    while (same && sqlite3_step(st) == SQLITE_ROW) {
        char sql[300];
        snprintf(sql, sizeof(sql), "SELECT count(*) FROM \"%s\"", (const char *)sqlite3_column_text(st, 0));
        same = scalar(src, sql, &a) == 0 && scalar(copy, sql, &b) == 0 && a == b;
    }
    sqlite3_finalize(st);
    return same;
}

static int fail(char *why, unsigned long size, const char *text) {
    if (why) str_copy(why, size, text);
    return -1;
}

static void sync_file(const char *path) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd >= 0) { fsync(fd); close(fd); }
}

/* Overwrites a file that held plain text before removing it. On SSDs and
 * copy-on-write filesystems the old blocks may survive anyway; full-disk
 * encryption is the answer to that, and the manual says so. */
static void overwrite_and_remove(const char *path) {
    int fd = open(path, O_WRONLY | O_CLOEXEC | O_NOFOLLOW);
    struct stat st;
    if (fd >= 0 && fstat(fd, &st) == 0) {
        char zeros[65536];
        memset(zeros, 0, sizeof(zeros));
        for (off_t done = 0; done < st.st_size;) {
            size_t chunk = (size_t)(st.st_size - done) < sizeof(zeros) ? (size_t)(st.st_size - done) : sizeof(zeros);
            ssize_t n = write(fd, zeros, chunk);
            if (n <= 0) break;
            done += n;
        }
        fsync(fd);
    }
    if (fd >= 0) close(fd);
    unlink(path);
}

static void remove_sidecar_files(const char *path) {
    char side[1100];
    snprintf(side, sizeof(side), "%s-wal", path);
    unlink(side);
    snprintf(side, sizeof(side), "%s-shm", path);
    unlink(side);
}

int sqlite_database_reencrypt(const char *path, const Passphrase *old_key, const Passphrase *new_key,
                              char *why, unsigned long why_size) {
    if (!sqlite_key_supported()) return fail(why, why_size, "this build of tawk has no SQLCipher");
    sqlite3 *src = open_with(path, old_key);
    if (!src) return fail(why, why_size, old_key ? "the passphrase is wrong" : "the database cannot be read");
    sqlite3_exec(src, "PRAGMA wal_checkpoint(TRUNCATE);", NULL, NULL, NULL);

    char next[1100], previous[1100];
    snprintf(next, sizeof(next), "%s.rewriting", path);
    snprintf(previous, sizeof(previous), "%s.previous", path);
    int fd = open(next, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) {
        sqlite3_close(src);
        return fail(why, why_size, "a leftover copy is in the way (remove tawk.db.rewriting)");
    }
    close(fd);

    long long version = 0;
    scalar(src, "PRAGMA user_version", &version);
    sqlite3_stmt *attach = NULL;
    int ok = sqlite3_prepare_v2(src, "ATTACH DATABASE ?1 AS target KEY ?2", -1, &attach, NULL) == SQLITE_OK;
    if (ok) {
        sqlite3_bind_text(attach, 1, next, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(attach, 2, new_key ? new_key->text : "", new_key ? (int)new_key->length : 0, SQLITE_TRANSIENT);
        ok = sqlite3_step(attach) == SQLITE_DONE;
    }
    sqlite3_finalize(attach);
    char pragma[64];
    snprintf(pragma, sizeof(pragma), "PRAGMA target.user_version = %lld;", version);
    ok = ok && sqlite3_exec(src, "SELECT sqlcipher_export('target');", NULL, NULL, NULL) == SQLITE_OK &&
         sqlite3_exec(src, pragma, NULL, NULL, NULL) == SQLITE_OK;
    sqlite3_exec(src, "DETACH DATABASE target;", NULL, NULL, NULL);

    sqlite3 *copy = ok ? open_with(next, new_key) : NULL;
    ok = copy && same_contents(src, copy);
    sqlite3_close(copy);
    sqlite3_close(src);
    if (!ok) {
        unlink(next);
        remove_sidecar_files(next);
        LOG_ERROR("database rewrite failed; the original is unchanged");
        return fail(why, why_size, "the new copy did not match the original, which is unchanged");
    }
    sync_file(next);

    /* Swap: the original steps aside first, so there is always a whole database. */
    if (rename(path, previous) != 0) { unlink(next); return fail(why, why_size, "the database could not be replaced"); }
    if (rename(next, path) != 0) {
        rename(previous, path);
        unlink(next);
        return fail(why, why_size, "the database could not be replaced");
    }
    remove_sidecar_files(path);
    remove_sidecar_files(next);
    if (old_key) unlink(previous);
    else overwrite_and_remove(previous);
    LOG_INFO("database %s", new_key ? (old_key ? "passphrase changed" : "encrypted") : "decrypted");
    return 0;
}

/* ---- IDatabaseCipher -------------------------------------------------------- */

static int c_supported(IDatabaseCipher *self) { (void)self; return sqlite_key_supported(); }
static int c_is_encrypted(IDatabaseCipher *self, const char *p) { (void)self; return sqlite_database_is_encrypted(p); }
static int c_unlocks(IDatabaseCipher *self, const char *p, const Passphrase *k) { (void)self; return sqlite_database_check_key(p, k); }

static int c_reencrypt(IDatabaseCipher *self, const char *p, const Passphrase *old_key, const Passphrase *new_key,
                       char *why, unsigned long why_size) {
    (void)self;
    return sqlite_database_reencrypt(p, old_key, new_key, why, why_size);
}

static int c_plain_copies(IDatabaseCipher *self, const char *db_path, char out[][DATABASE_COPY_PATH_MAX], int max) {
    (void)self;
    char dir[700], prefix[300];
    str_copy(dir, sizeof(dir), db_path);
    char *slash = strrchr(dir, '/');
    const char *base = slash ? slash + 1 : db_path;
    snprintf(prefix, sizeof(prefix), "%s.pre-v", base);
    if (slash) *slash = '\0'; else str_copy(dir, sizeof(dir), ".");
    DIR *d = opendir(dir);
    if (!d) return 0;
    int n = 0;
    struct dirent *e;
    while (n < max && (e = readdir(d)) != NULL) {
        if (strncmp(e->d_name, prefix, strlen(prefix)) != 0) continue;
        char path[DATABASE_COPY_PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
        struct stat st;
        if (lstat(path, &st) != 0 || !S_ISREG(st.st_mode) || sqlite_database_is_encrypted(path)) continue;
        str_copy(out[n++], DATABASE_COPY_PATH_MAX, path);
    }
    closedir(d);
    return n;
}

static int c_shred(IDatabaseCipher *self, const char *path) {
    (void)self;
    overwrite_and_remove(path);
    remove_sidecar_files(path);
    return access(path, F_OK) == 0 ? -1 : 0;
}

static void c_destroy(IDatabaseCipher *self) { free(self); }

IDatabaseCipher *sqlite_database_cipher_create(void) {
    IDatabaseCipher *c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->supported = c_supported;
    c->is_encrypted = c_is_encrypted;
    c->unlocks = c_unlocks;
    c->reencrypt = c_reencrypt;
    c->plain_copies = c_plain_copies;
    c->shred = c_shred;
    c->destroy = c_destroy;
    return c;
}
