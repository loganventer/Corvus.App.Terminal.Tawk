#ifndef APP_UTILITIES_DROPPED_PATH_H
#define APP_UTILITIES_DROPPED_PATH_H

#include <stddef.h>

/* Terminals "drop" files by pasting their path. Converts what was pasted
 * into a local path: strips quotes, decodes file:// URIs and backslash
 * escapes, and maps Windows paths (C:\...) to /mnt/c/... under WSL.
 * Returns 0 only when the result is an existing regular file. */
int dropped_path_resolve(const char *pasted, char *out, size_t size);

#endif
