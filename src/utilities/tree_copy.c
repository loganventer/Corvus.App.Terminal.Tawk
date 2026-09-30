#include "utilities/tree_copy.h"
#include "utilities/file_copy.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int skip(const char *name) { return !strcmp(name, ".") || !strcmp(name, ".."); }

int tree_copy(const char *src, const char *dst) {
    if (mkdir(dst, 0700) != 0) return -1;
    DIR *d = opendir(src);
    if (!d) return -1;
    int rc = 0;
    struct dirent *e;
    while (rc == 0 && (e = readdir(d)) != NULL) {
        if (skip(e->d_name)) continue;
        char from[4096], to[4096];
        snprintf(from, sizeof(from), "%s/%s", src, e->d_name);
        snprintf(to, sizeof(to), "%s/%s", dst, e->d_name);
        struct stat st;
        if (lstat(from, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) rc = tree_copy(from, to);
        else if (S_ISREG(st.st_mode) && link(from, to) != 0) rc = file_copy(from, to, 0600);
    }
    closedir(d);
    return rc;
}

void tree_make_private(const char *root) {
    struct stat st;
    if (lstat(root, &st) != 0) return;
    if (S_ISREG(st.st_mode)) { chmod(root, 0600); return; }
    if (!S_ISDIR(st.st_mode)) return;
    chmod(root, 0700);
    DIR *d = opendir(root);
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (skip(e->d_name)) continue;
        char path[4096];
        snprintf(path, sizeof(path), "%s/%s", root, e->d_name);
        tree_make_private(path);
    }
    closedir(d);
}

int tree_remove(const char *root) {
    struct stat st;
    if (lstat(root, &st) != 0) return errno == ENOENT ? 0 : -1;
    if (!S_ISDIR(st.st_mode)) return unlink(root);
    DIR *d = opendir(root);
    if (!d) return -1;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (skip(e->d_name)) continue;
        char path[4096];
        snprintf(path, sizeof(path), "%s/%s", root, e->d_name);
        tree_remove(path);
    }
    closedir(d);
    return rmdir(root);
}
