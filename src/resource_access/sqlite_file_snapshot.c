#include "resource_access/sqlite_file_snapshot.h"
#include "utilities/file_copy.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

static int take(IDatabaseSnapshot *self, const char *db_path, const char *dest_dir) {
    (void)self;
    char dest[1200], wal[1100], dest_wal[1300];
    int n = snprintf(dest, sizeof(dest), "%s/tawk.db", dest_dir);
    if (n < 0 || (size_t)n >= sizeof(dest) || file_copy(db_path, dest, 0600) != 0) return -1;
    /* A log left by a tawk that did not close cleanly holds the newest changes. */
    n = snprintf(wal, sizeof(wal), "%s-wal", db_path);
    if (n < 0 || (size_t)n >= sizeof(wal)) return -1;
    snprintf(dest_wal, sizeof(dest_wal), "%s-wal", dest);
    struct stat st;
    if (lstat(wal, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0 && file_copy(wal, dest_wal, 0600) != 0) return -1;
    return 0;
}

static void destroy(IDatabaseSnapshot *self) { free(self); }

IDatabaseSnapshot *sqlite_file_snapshot_create(void) {
    IDatabaseSnapshot *s = calloc(1, sizeof(*s));
    if (!s) return NULL;
    s->take = take;
    s->destroy = destroy;
    return s;
}
