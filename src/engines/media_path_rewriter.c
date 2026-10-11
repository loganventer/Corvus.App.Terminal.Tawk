#include "engines/media_path_rewriter.h"

#include <stdio.h>
#include <string.h>

/* True when a component of `rel` is "..". */
static int climbs(const char *rel) {
    for (const char *p = rel; *p;) {
        const char *slash = strchr(p, '/');
        size_t len = slash ? (size_t)(slash - p) : strlen(p);
        if (len == 2 && p[0] == '.' && p[1] == '.') return 1;
        p = slash ? slash + 1 : p + len;
    }
    return 0;
}

int media_path_relative(const char *dir, const char *path, char *out, size_t size) {
    if (!dir || !*dir || !path || !out || size == 0) return -1;
    size_t n = strlen(dir);
    while (n > 1 && dir[n - 1] == '/') n--;
    if (strncmp(path, dir, n) != 0 || path[n] != '/') return -1;
    const char *rel = path + n + 1;
    if (!*rel || climbs(rel) || strlen(rel) >= size) return -1;
    memcpy(out, rel, strlen(rel) + 1);
    return 0;
}

int media_path_rewrite(const MediaPathMap *map, const char *path, char *out, size_t size) {
    char rel[1024];
    if (!map || !map->keep || !map->new_dir || !*map->new_dir || !path || !*path) return -1;
    if (media_path_relative(map->old_dir, path, rel, sizeof(rel)) != 0) return -1;
    size_t n = strlen(map->new_dir);
    while (n > 1 && map->new_dir[n - 1] == '/') n--;
    int w = snprintf(out, size, "%.*s/%s", (int)n, map->new_dir, rel);
    return w > 0 && (size_t)w < size ? 0 : -1;
}
