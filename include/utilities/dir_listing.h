#ifndef APP_UTILITIES_DIR_LISTING_H
#define APP_UTILITIES_DIR_LISTING_H

#include "utilities/file_entry.h"

/* Lists `dir`: folders first, then files, each sorted case-insensitively.
 * Hidden entries (leading dot) are skipped unless show_hidden. Symbolic links
 * are followed for type and size. Returns the count (0 on error) and a
 * malloc'd array the caller frees. */
int dir_listing_read(const char *dir, int show_hidden, FileEntry **out);

#endif
