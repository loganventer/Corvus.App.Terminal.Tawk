#include "utilities/dir_listing.h"
#include "utilities/str_util.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

static int compare(const void *a, const void *b) {
    const FileEntry *x = a, *y = b;
    if (x->is_dir != y->is_dir) return y->is_dir - x->is_dir;
    return strcasecmp(x->name, y->name);
}

int dir_listing_read(const char *dir, int show_hidden, FileEntry **out) {
    *out = NULL;
    DIR *d = opendir(dir);
    if (!d) return 0;
    int fd = dirfd(d), count = 0, cap = 128;
    FileEntry *items = malloc((size_t)cap * sizeof(FileEntry));
    struct dirent *ent;
    while (items && (ent = readdir(d)) != NULL) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
        if (ent->d_name[0] == '.' && !show_hidden) continue;
        struct stat st;
        if (fstatat(fd, ent->d_name, &st, 0) != 0) continue;
        if (!S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode)) continue;
        if (count == cap) {
            FileEntry *grown = realloc(items, (size_t)cap * 2 * sizeof(FileEntry));
            if (!grown) break;
            items = grown;
            cap *= 2;
        }
        str_copy(items[count].name, sizeof(items[count].name), ent->d_name);
        items[count].is_dir = S_ISDIR(st.st_mode);
        items[count].size = (long long)st.st_size;
        count++;
    }
    closedir(d);
    if (!items) return 0;
    qsort(items, (size_t)count, sizeof(FileEntry), compare);
    *out = items;
    return count;
}
