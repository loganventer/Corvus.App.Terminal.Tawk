#include "infrastructure/media_cache_janitor.h"
#include "utilities/log.h"
#include "utilities/path_util.h"
#include "utilities/str_util.h"

#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct CachedFile {
    char   name[256];
    time_t mtime;
    off_t  size;
} CachedFile;

static int oldest_first(const void *a, const void *b) {
    const CachedFile *x = a, *y = b;
    return (x->mtime > y->mtime) - (x->mtime < y->mtime);
}

int media_cache_janitor_prune(const char *dir, int max_mb) {
    if (max_mb <= 0) return 0;
    DIR *d = opendir(dir);
    if (!d) return 0;
    int dir_fd = dirfd(d);
    int cap = 256, count = 0;
    CachedFile *files = malloc((size_t)cap * sizeof(CachedFile));
    long long total = 0;
    struct dirent *ent;
    while (files && (ent = readdir(d)) != NULL) {
        struct stat st;
        if (ent->d_name[0] == '.' || strlen(ent->d_name) >= sizeof(files[0].name)) continue;
        if (fstatat(dir_fd, ent->d_name, &st, AT_SYMLINK_NOFOLLOW) != 0 || !S_ISREG(st.st_mode)) continue;
        if (count == cap) {
            CachedFile *grown = realloc(files, (size_t)cap * 2 * sizeof(CachedFile));
            if (!grown) break;
            files = grown;
            cap *= 2;
        }
        str_copy(files[count].name, sizeof(files[count].name), ent->d_name);
        files[count].mtime = st.st_mtime;
        files[count].size = st.st_size;
        total += st.st_size;
        count++;
    }
    long long limit = (long long)max_mb * 1024 * 1024;
    int removed = 0;
    if (files && total > limit) {
        qsort(files, (size_t)count, sizeof(CachedFile), oldest_first);
        for (int i = 0; i < count && total > limit; i++) {
            if (unlinkat(dir_fd, files[i].name, 0) == 0) {
                total -= files[i].size;
                removed++;
            }
        }
        LOG_INFO("media cache: removed %d old file(s) to stay under %d MB", removed, max_mb);
    }
    free(files);
    closedir(d);
    return removed;
}
