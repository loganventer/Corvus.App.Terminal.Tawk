#ifndef APP_ENGINES_MEDIA_PATH_REWRITER_H
#define APP_ENGINES_MEDIA_PATH_REWRITER_H

#include <stddef.h>

#include "core/media_path_map.h"

/* Where a downloaded file is after its account moved. 0 and `out` filled in when the path was inside the
 * old media folder and the files came along; -1 when it should be cleared (the file did not come, or the
 * path was somewhere else), so tawk fetches it again from WhatsApp when it is opened. */
int media_path_rewrite(const MediaPathMap *map, const char *path, char *out, size_t size);
/* The part of `path` below `dir`, with no ".." in it; 0 when it is inside, -1 otherwise. */
int media_path_relative(const char *dir, const char *path, char *out, size_t size);

#endif
