#ifndef APP_UTILITIES_FILE_INSPECTOR_H
#define APP_UTILITIES_FILE_INSPECTOR_H

#include "utilities/file_facts.h"

#include <stddef.h>

/* Looks at `path`, following links. Never fails: a missing path has exists 0. */
void file_inspector_read(const char *path, FileFacts *out);
/* The absolute path with links and ".." resolved; 0 on success, -1 when the path does not exist. */
int  file_inspector_resolve(const char *path, char *out, size_t size);

#endif
