#include "utilities/outgoing_media.h"
#include "utilities/path_util.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_SEND_BYTES (100L * 1024 * 1024)

/* Copies src to dst (0600, never following links); refuses large files. */
static int copy_file(const char *src, const char *dst) {
    int in = open(src, O_RDONLY | O_CLOEXEC);
    if (in < 0) return -1;
    struct stat st;
    if (fstat(in, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0 || st.st_size > MAX_SEND_BYTES) {
        close(in);
        return -1;
    }
    int out = open(dst, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (out < 0) { close(in); return -1; }
    char buf[65536];
    ssize_t n;
    int rc = 0;
    while ((n = read(in, buf, sizeof(buf))) > 0) {
        if (write(out, buf, (size_t)n) != n) { rc = -1; break; }
    }
    if (n < 0) rc = -1;
    close(in);
    close(out);
    if (rc != 0) unlink(dst);
    return rc;
}

/* The source's extension, dot included, so the copy keeps its file type. */
static const char *extension_of(const char *path) {
    const char *slash = strrchr(path, '/');
    const char *dot = strrchr(path, '.');
    return (dot && (!slash || dot > slash)) ? dot : "";
}

int outgoing_media_copy(const char *media_dir, const char *source, const char *id, char *out, size_t out_size) {
    if (!media_dir || !source || !id || !*id || !out || out_size == 0) return -1;
    char dir[512];
    path_join(dir, sizeof(dir), media_dir, "outgoing");
    if (path_mkdir_p(dir, 0700) != 0) return -1;
    int n = snprintf(out, out_size, "%s/%s%s", dir, id, extension_of(source));
    if (n < 0 || (size_t)n >= out_size) return -1;
    if (copy_file(source, out) != 0) { out[0] = '\0'; return -1; }
    return 0;
}
