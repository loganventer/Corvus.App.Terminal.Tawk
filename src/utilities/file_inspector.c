#include "utilities/file_inspector.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void file_inspector_read(const char *path, FileFacts *out) {
    struct stat st;
    memset(out, 0, sizeof(*out));
    if (!path || !*path || stat(path, &st) != 0) return;
    out->exists = 1;
    out->is_regular = S_ISREG(st.st_mode) ? 1 : 0;
    out->size_bytes = (long long)st.st_size;
}

int file_inspector_resolve(const char *path, char *out, size_t size) {
    char real[PATH_MAX];
    if (!path || !*path || !out || size == 0 || !realpath(path, real)) return -1;
    if (strlen(real) >= size) return -1;
    memcpy(out, real, strlen(real) + 1);
    return 0;
}
