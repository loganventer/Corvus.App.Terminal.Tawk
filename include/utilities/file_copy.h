#ifndef APP_UTILITIES_FILE_COPY_H
#define APP_UTILITIES_FILE_COPY_H

#include <stddef.h>

/* Copies a regular file to `dst`, which must not exist yet (never follows
 * or replaces anything there). The copy is private (0600) unless `mode`
 * says otherwise. Returns 0 on success; a partial copy is removed. */
int file_copy(const char *src, const char *dst, int mode);
/* A path in `dir` for `name` that does not exist yet: "name.ext", then
 * "name (1).ext", "name (2).ext", ... */
int file_unique_path(const char *dir, const char *name, char *out, size_t size);

#endif
