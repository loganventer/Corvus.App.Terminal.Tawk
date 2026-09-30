#include "utilities/file_copy.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int file_copy(const char *src, const char *dst, int mode) {
    int in = open(src, O_RDONLY | O_CLOEXEC);
    if (in < 0) return -1;
    struct stat st;
    if (fstat(in, &st) != 0 || !S_ISREG(st.st_mode)) { close(in); return -1; }
    int out = open(dst, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, mode ? mode : 0600);
    if (out < 0) { close(in); return -1; }
    char buf[65536];
    ssize_t n;
    int rc = 0;
    while ((n = read(in, buf, sizeof(buf))) > 0) {
        if (write(out, buf, (size_t)n) != n) { rc = -1; break; }
    }
    if (n < 0) rc = -1;
    close(in);
    if (close(out) != 0) rc = -1;
    if (rc != 0) unlink(dst);
    return rc;
}

int file_unique_path(const char *dir, const char *name, char *out, size_t size) {
    const char *dot = strrchr(name, '.');
    size_t stem = dot && dot != name ? (size_t)(dot - name) : strlen(name);
    const char *ext = dot && dot != name ? dot : "";
    for (int i = 0; i < 1000; i++) {
        int n = i == 0 ? snprintf(out, size, "%s/%s", dir, name)
                       : snprintf(out, size, "%s/%.*s (%d)%s", dir, (int)stem, name, i, ext);
        if (n < 0 || (size_t)n >= size) return -1;
        if (access(out, F_OK) != 0) return 0;
    }
    return -1;
}
