#include "infrastructure/osc52_clipboard.h"
#include "utilities/base64.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_COPY_BYTES (64 * 1024)

static int clip_copy(IClipboard *self, const char *utf8) {
    (void)self;
    if (!utf8) return -1;
    size_t len = strlen(utf8);
    if (len > MAX_COPY_BYTES) len = MAX_COPY_BYTES;
    char *encoded = base64_encode((const unsigned char *)utf8, len);
    if (!encoded) return -1;
    int fd = open("/dev/tty", O_WRONLY | O_CLOEXEC | O_NOCTTY);
    int rc = -1;
    if (fd >= 0) {
        ssize_t a = write(fd, "\033]52;c;", 7);
        ssize_t b = write(fd, encoded, strlen(encoded));
        ssize_t c = write(fd, "\007", 1);
        rc = (a == 7 && b == (ssize_t)strlen(encoded) && c == 1) ? 0 : -1;
        close(fd);
    }
    free(encoded);
    return rc;
}

static void clip_destroy(IClipboard *self) { free(self); }

IClipboard *osc52_clipboard_create(void) {
    IClipboard *c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->copy = clip_copy;
    c->destroy = clip_destroy;
    return c;
}
