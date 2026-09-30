#include "infrastructure/unix_control_transport.h"
#include "utilities/log.h"
#include "utilities/path_util.h"
#include "utilities/socket_io.h"
#include "utilities/str_util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define MAX_CLIENTS   16
#define MAX_LINE      (1024 * 1024)
#define MAX_OUTBOX    (8 * 1024 * 1024)   /* a client this far behind is dropped */

typedef struct Buffer {
    char  *data;
    size_t len;
    size_t cap;
} Buffer;

typedef struct Client {
    int    fd;              /* -1 when the slot is free */
    int    id;
    int    announced;       /* OPENED was reported */
    int    closing;         /* CLOSED is still to be reported */
    Buffer in;
    Buffer out;
} Client;

typedef struct Transport {
    int    listen_fd;
    char   path[512];
    int    next_id;
    Client clients[MAX_CLIENTS];
} Transport;

static int buffer_append(Buffer *b, const char *data, size_t n) {
    if (b->len + n + 1 > b->cap) {
        size_t cap = b->cap ? b->cap : 4096;
        while (cap < b->len + n + 1) cap *= 2;
        char *bigger = realloc(b->data, cap);
        if (!bigger) return -1;
        b->data = bigger;
        b->cap = cap;
    }
    memcpy(b->data + b->len, data, n);
    b->len += n;
    b->data[b->len] = '\0';
    return 0;
}

static void buffer_consume(Buffer *b, size_t n) {
    memmove(b->data, b->data + n, b->len - n);
    b->len -= n;
    if (b->data) b->data[b->len] = '\0';
}

static void buffer_free(Buffer *b) {
    free(b->data);
    memset(b, 0, sizeof(*b));
}

static int same_user(int fd) {
#if defined(__APPLE__) || defined(__FreeBSD__)
    uid_t uid;
    gid_t gid;
    return getpeereid(fd, &uid, &gid) == 0 && uid == getuid();
#else
    struct ucred cred;
    socklen_t len = sizeof(cred);
    return getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &len) == 0 && cred.uid == getuid();
#endif
}

static void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    fcntl(fd, F_SETFD, FD_CLOEXEC);
}

/* Closes the connection. Lines already received are still handed over
 * when `keep_input` (the client sent them, then hung up). */
static void hang_up(Client *c, int keep_input) {
    if (c->fd >= 0) close(c->fd);
    c->fd = -1;
    if (!keep_input) buffer_free(&c->in);
    buffer_free(&c->out);
    c->closing = c->announced;
}

static void drop(Client *c) { hang_up(c, 0); }

static Client *find(Transport *t, int conn) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (t->clients[i].fd >= 0 && t->clients[i].id == conn) return &t->clients[i];
    }
    return NULL;
}

static void flush(Client *c) {
    while (c->fd >= 0 && c->out.len > 0) {
        ssize_t n = socket_send(c->fd, c->out.data, c->out.len);
        if (n > 0) { buffer_consume(&c->out, (size_t)n); continue; }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
        if (n < 0 && errno == EINTR) continue;
        drop(c);
    }
}

/* A socket file left by a tawk that did not quit cleanly is removed; one
 * that still answers belongs to a tawk that is running. */
static int claim_path(const char *path, char *why, unsigned long why_size) {
    struct stat st;
    if (lstat(path, &st) != 0) return 0;
    if (!S_ISSOCK(st.st_mode)) {
        snprintf(why, why_size, "%s exists and is not a socket", path);
        return -1;
    }
    int probe = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    str_copy(addr.sun_path, sizeof(addr.sun_path), path);
    int answered = probe >= 0 && connect(probe, (struct sockaddr *)&addr, sizeof(addr)) == 0;
    if (probe >= 0) close(probe);
    if (answered) {
        snprintf(why, why_size, "another program is listening on %s", path);
        return -1;
    }
    unlink(path);
    return 0;
}

static void tr_stop(IControlTransport *self) {
    Transport *t = self->ctx;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (t->clients[i].fd >= 0) drop(&t->clients[i]);
        t->clients[i].closing = 0;
    }
    if (t->listen_fd >= 0) {
        close(t->listen_fd);
        t->listen_fd = -1;
        unlink(t->path);
    }
    t->path[0] = '\0';
}

static int tr_listen(IControlTransport *self, const char *path, char *why, unsigned long why_size) {
    Transport *t = self->ctx;
    tr_stop(self);
    struct sockaddr_un addr;
    if (strlen(path) >= sizeof(addr.sun_path)) {
        snprintf(why, why_size, "the socket path is too long: %s", path);
        return -1;
    }
    char dir[512];
    str_copy(dir, sizeof(dir), path);
    char *slash = strrchr(dir, '/');
    if (slash) {
        *slash = '\0';
        if (path_mkdir_p(dir, 0700) != 0) {
            snprintf(why, why_size, "cannot create %s: %s", dir, strerror(errno));
            return -1;
        }
        chmod(dir, 0700);
    }
    if (claim_path(path, why, why_size) != 0) return -1;
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        snprintf(why, why_size, "cannot create a socket: %s", strerror(errno));
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    str_copy(addr.sun_path, sizeof(addr.sun_path), path);
    mode_t old = umask(077);
    int bound = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    umask(old);
    if (bound != 0 || chmod(path, 0600) != 0 || listen(fd, 8) != 0) {
        snprintf(why, why_size, "cannot listen on %s: %s", path, strerror(errno));
        close(fd);
        return -1;
    }
    set_nonblocking(fd);
    t->listen_fd = fd;
    str_copy(t->path, sizeof(t->path), path);
    LOG_INFO("control socket listening on %s", path);
    return 0;
}

static void accept_new(Transport *t) {
    for (;;) {
        int fd = accept(t->listen_fd, NULL, NULL);
        if (fd < 0) return;
        set_nonblocking(fd);
        socket_no_sigpipe(fd);
        Client *slot = NULL;
        for (int i = 0; i < MAX_CLIENTS && !slot; i++) {
            if (t->clients[i].fd < 0 && !t->clients[i].closing) slot = &t->clients[i];
        }
        if (!slot || !same_user(fd)) {
            LOG_WARN("control socket: %s", slot ? "refused a client of another user" : "too many clients");
            close(fd);
            continue;
        }
        memset(slot, 0, sizeof(*slot));
        slot->fd = fd;
        slot->id = ++t->next_id;
    }
}

static void read_available(Client *c) {
    char chunk[16384];
    while (c->fd >= 0) {
        ssize_t n = recv(c->fd, chunk, sizeof(chunk), 0);
        if (n > 0) {
            if (buffer_append(&c->in, chunk, (size_t)n) != 0) { drop(c); return; }
            if (c->in.len > MAX_LINE && !memchr(c->in.data, '\n', c->in.len)) {
                LOG_WARN("control socket: a line longer than 1 MiB; closing that client");
                drop(c);
            }
            continue;
        }
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
        hang_up(c, n == 0);                     /* end of file, or an error */
    }
}

static int tr_poll(IControlTransport *self, ControlInbound *out, int max) {
    Transport *t = self->ctx;
    int n = 0;
    if (t->listen_fd >= 0) accept_new(t);
    for (int i = 0; i < MAX_CLIENTS && n < max; i++) {
        Client *c = &t->clients[i];
        if (c->fd >= 0 && !c->announced) {
            c->announced = 1;
            out[n++] = (ControlInbound){ CONTROL_INBOUND_OPENED, c->id, NULL };
        }
        if (c->fd >= 0) {
            flush(c);
            read_available(c);
        }
        while (n < max && c->in.len > 0) {
            char *nl = memchr(c->in.data, '\n', c->in.len);
            if (!nl) break;
            size_t len = (size_t)(nl - c->in.data);
            char *line = malloc(len + 1);
            if (!line) break;
            memcpy(line, c->in.data, len);
            line[len] = '\0';
            if (len > 0 && line[len - 1] == '\r') line[len - 1] = '\0';
            buffer_consume(&c->in, len + 1);
            if (line[0]) out[n++] = (ControlInbound){ CONTROL_INBOUND_LINE, c->id, line };
            else free(line);
        }
        if (c->fd < 0 && c->closing && !memchr(c->in.data ? c->in.data : "", '\n', c->in.len) && n < max) {
            c->closing = 0;
            buffer_free(&c->in);
            out[n++] = (ControlInbound){ CONTROL_INBOUND_CLOSED, c->id, NULL };
        }
    }
    return n;
}

static int tr_send(IControlTransport *self, int conn, const char *line) {
    Transport *t = self->ctx;
    Client *c = find(t, conn);
    if (!c) return -1;
    if (c->out.len > MAX_OUTBOX) {
        LOG_WARN("control socket: a client stopped reading; closing it");
        drop(c);
        return -1;
    }
    if (buffer_append(&c->out, line, strlen(line)) != 0 || buffer_append(&c->out, "\n", 1) != 0) return -1;
    flush(c);
    return c->fd >= 0 ? 0 : -1;
}

static void tr_close_conn(IControlTransport *self, int conn) {
    Client *c = find(self->ctx, conn);
    if (!c) return;
    flush(c);
    drop(c);
    c->closing = 0;                             /* the owner asked for it; no CLOSED */
}

static void tr_destroy(IControlTransport *self) {
    tr_stop(self);
    free(self->ctx);
    free(self);
}

IControlTransport *unix_control_transport_create(void) {
    IControlTransport *s = calloc(1, sizeof(*s));
    Transport *t = calloc(1, sizeof(*t));
    if (!s || !t) { free(s); free(t); return NULL; }
    t->listen_fd = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) t->clients[i].fd = -1;
    s->ctx = t;
    s->listen = tr_listen;
    s->stop = tr_stop;
    s->poll = tr_poll;
    s->send = tr_send;
    s->close_conn = tr_close_conn;
    s->destroy = tr_destroy;
    return s;
}
