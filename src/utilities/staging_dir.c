#include "utilities/staging_dir.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int staging_dir_make(const char *parent, const char *kind, char *out, size_t size) {
    int n = snprintf(out, size, "%s/.%s-XXXXXX", parent, kind);
    if (n < 0 || (size_t)n >= size) return -1;
    return mkdtemp(out) ? 0 : -1;
}
