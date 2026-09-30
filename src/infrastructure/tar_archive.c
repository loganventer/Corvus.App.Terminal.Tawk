#include "infrastructure/tar_archive.h"
#include "utilities/process_capture.h"
#include "utilities/process_quiet.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LIST_TIMEOUT_MS (10 * 60 * 1000)   /* a large media folder takes a while */
#define MAX_NAMES       4096

static int create(IArchive *self, const char *root, const char *const *names, int count, const char *archive_path) {
    (void)self;
    if (count <= 0 || count > MAX_NAMES) return -1;
    char **argv = calloc((size_t)count + 8, sizeof(*argv));
    if (!argv) return -1;
    int n = 0;
    argv[n++] = "tar";
    argv[n++] = "-czf";
    argv[n++] = (char *)archive_path;
    argv[n++] = "-C";
    argv[n++] = (char *)root;
    argv[n++] = "--";
    for (int i = 0; i < count; i++) argv[n++] = (char *)names[i];
    argv[n] = NULL;
    int rc = process_run_quiet(argv, NULL, 0, -1);
    free(argv);
    return rc == 0 ? 0 : -1;
}

/* Runs tar with `option` (-tzf or -tzvf) and reads one line per member
 * into `lines` (each at most `width` bytes); returns how many, or -1. */
static int list_lines(const char *archive_path, const char *option, char *lines, size_t width, int max) {
    char tmp[] = "/tmp/tawk-list-XXXXXX";
    int fd = mkstemp(tmp);
    if (fd < 0) return -1;
    unlink(tmp);
    char *argv[] = { "tar", (char *)option, (char *)archive_path, NULL };
    if (process_run_to_fd(argv, fd, LIST_TIMEOUT_MS) != 0) { close(fd); return -1; }
    FILE *f = fdopen(fd, "r");
    if (!f) { close(fd); return -1; }
    rewind(f);
    int n = 0;
    char line[4096];
    while (fgets(line, sizeof(line), f)) {
        if (n >= max) { n = -1; break; }                       /* more members than a backup can have */
        line[strcspn(line, "\n")] = '\0';
        str_copy(lines + (size_t)n * width, width, line);
        n++;
    }
    fclose(f);
    return n;
}

static int list(IArchive *self, const char *archive_path, ArchiveEntry *out, int max) {
    (void)self;
    size_t width = sizeof(out[0].name);
    char *names = calloc((size_t)max, width), *long_form = calloc((size_t)max, width);
    int count = -1;
    if (names && long_form) {
        int n = list_lines(archive_path, "-tzf", names, width, max);
        int m = list_lines(archive_path, "-tzvf", long_form, width, max);
        /* The long listing gives each member's type as its first character;
         * the two must describe the same members, one per line. */
        if (n >= 0 && n == m) {
            for (int i = 0; i < n; i++) {
                str_copy(out[i].name, width, names + (size_t)i * width);
                out[i].type = long_form[(size_t)i * width];
            }
            count = n;
        }
    }
    free(names);
    free(long_form);
    return count;
}

static int extract(IArchive *self, const char *archive_path, const char *dest_dir) {
    (void)self;
    char *argv[] = { "tar", "-xzf", (char *)archive_path, "-C", (char *)dest_dir, "--no-same-owner", "--no-same-permissions", NULL };
    return process_run_quiet(argv, NULL, 0, -1) == 0 ? 0 : -1;
}

static void destroy(IArchive *self) { free(self); }

IArchive *tar_archive_create(void) {
    IArchive *a = calloc(1, sizeof(*a));
    if (!a) return NULL;
    a->create = create;
    a->list = list;
    a->extract = extract;
    a->destroy = destroy;
    return a;
}
