#ifndef APP_UTILITIES_STAGING_DIR_H
#define APP_UTILITIES_STAGING_DIR_H

#include <stddef.h>

/* A private working folder (0700) named ".KIND-XXXXXX" inside `parent`, so
 * renames and hard links stay on one filesystem. Returns 0 and its path in
 * `out`, or -1. The caller removes it with tree_remove. */
int staging_dir_make(const char *parent, const char *kind, char *out, size_t size);

#endif
