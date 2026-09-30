#include "infrastructure/unix_control_client.h"
#include "utilities/socket_io.h"
#include "utilities/str_util.h"

#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

typedef struct Client {
    int    fd;
    char  *buf;
    size_t len;
    size_t cap;
} Client;

static int cl_connect(IControlClient *self, const char *path) {
    Client *c = self->ctx;
    struct sockaddr_un addr;
    if (strlen(path) >= sizeof(addr.sun_path)) return -1;
    c->fd = socket_unix_stream();
    if (c->fd < 0) return -1;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    str_copy(addr.sun_path, sizeof(addr.sun_path), path);
    if (connect(c->fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(c->fd);
        c->fd = -1;
        return -1;
    }
    return 0;
}

static int cl_send(IControlClient *self, const char *line) {
    Client *c = self->ctx;
    if (c->fd < 0) return -1;
    size_t n = strlen(line), done = 0;
    while (done <= n) {
        const char *p = done < n ? line + done : "\n";
        size_t left = done < n ? n - done : 1;
        ssize_t w = socket_send(c->fd, p, left);
        if (w < 0 && errno == EINTR) continue;
        if (w <= 0) return -1;
        done += (size_t)w;
        if (done == n + 1) break;
    }
    return 0;
}

/* The first complete line in the buffer, taken out of it. */
static char *take_line(Client *c) {
    char *nl = c->len ? memchr(c->buf, '\n', c->len) : NULL;
    if (!nl) return NULL;
    size_t len = (size_t)(nl - c->buf);
    char *line = malloc(len + 1);
    if (!line) return NULL;
    memcpy(line, c->buf, len);
    line[len] = '\0';
    memmove(c->buf, nl + 1, c->len - len - 1);
    c->len -= len + 1;
    return line;
}

static char *cl_read_line(IControlClient *self, int timeout_ms) {
    Client *c = self->ctx;
    for (;;) {
        char *line = take_line(c);
        if (line) return line;
        if (c->fd < 0) return NULL;
        struct pollfd p = { c->fd, POLLIN, 0 };
        int ready = poll(&p, 1, timeout_ms);
        if (ready < 0 && errno == EINTR) continue;
        if (ready <= 0) return NULL;
        if (c->len + 16384 > c->cap) {
            size_t cap = c->cap ? c->cap * 2 : 65536;
            char *bigger = realloc(c->buf, cap);
            if (!bigger) return NULL;
            c->buf = bigger;
            c->cap = cap;
        }
        ssize_t n = recv(c->fd, c->buf + c->len, c->cap - c->len, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { close(c->fd); c->fd = -1; continue; }
        c->len += (size_t)n;
    }
}

static void cl_destroy(IControlClient *self) {
    Client *c = self->ctx;
    if (c->fd >= 0) close(c->fd);
    free(c->buf);
    free(c);
    free(self);
}

IControlClient *unix_control_client_create(void) {
    IControlClient *s = calloc(1, sizeof(*s));
    Client *c = calloc(1, sizeof(*c));
    if (!s || !c) { free(s); free(c); return NULL; }
    c->fd = -1;
    s->ctx = c;
    s->connect = cl_connect;
    s->send = cl_send;
    s->read_line = cl_read_line;
    s->destroy = cl_destroy;
    return s;
}
