#ifndef APP_INFRASTRUCTURE_MEDIA_CACHE_JANITOR_H
#define APP_INFRASTRUCTURE_MEDIA_CACHE_JANITOR_H

/* Deletes the oldest regular files in `dir` (by modification time) until the
 * folder is at most `max_mb` megabytes. Subfolders and symlinks are left
 * alone. Returns the number of files removed. */
int media_cache_janitor_prune(const char *dir, int max_mb);

#endif
