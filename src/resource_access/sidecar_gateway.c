#include "resource_access/sidecar_gateway.h"
#include "resource_access/json_protocol.h"
#include "utilities/log.h"
#include "utilities/path_util.h"
#include "utilities/str_util.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_LINE_BYTES (2 * 1024 * 1024)

typedef struct Sidecar {
    GatewayOptions  options;
    EventQueue     *events;
    pid_t           pid;
    int             to_child;
    int             from_child;
    pthread_t       reader;
    int             reader_running;
    pthread_mutex_t write_lock;
    IProfileEditor  editor;         /* a view of this gateway, handed out by the accessor */
    IStatusLiker    liker;          /* likes of statuses, addressed to their author alone */
} Sidecar;

static Sidecar *ctx_of(IMessageGateway *self) { return (Sidecar *)self->ctx; }

static void push_simple(Sidecar *sc, EventType type, const char *detail) {
    Event evt;
    event_init(&evt, type);
    str_copy(evt.detail, sizeof(evt.detail), detail);
    if (event_queue_push(sc->events, &evt) != 0) event_dispose(&evt);
}

static void handle_line(Sidecar *sc, const char *line) {
    Event evt;
    if (json_protocol_decode(line, &evt) != 0) {
        LOG_DEBUG("sidecar: ignored line (%zu bytes)", strlen(line));
        return;
    }
    if (event_queue_push(sc->events, &evt) != 0) event_dispose(&evt);
}

static void *reader_main(void *arg) {
    Sidecar *sc = arg;
    size_t cap = 64 * 1024, len = 0;
    char *buf = malloc(cap);
    int discarding = 0;
    char chunk[16 * 1024];
    while (buf) {
        ssize_t n = read(sc->from_child, chunk, sizeof(chunk));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        for (ssize_t i = 0; i < n; i++) {
            if (chunk[i] == '\n') {
                if (!discarding) { buf[len] = '\0'; handle_line(sc, buf); }
                len = 0;
                discarding = 0;
                continue;
            }
            if (discarding) continue;
            if (len + 2 > cap) {
                if (cap >= MAX_LINE_BYTES) { discarding = 1; LOG_WARN("sidecar: oversized line dropped"); continue; }
                char *grown = realloc(buf, cap * 2);
                if (!grown) { discarding = 1; continue; }
                buf = grown;
                cap *= 2;
            }
            buf[len++] = chunk[i];
        }
    }
    free(buf);
    int status = 0;
    char detail[128] = "The WhatsApp bridge stopped.";
    if (sc->pid > 0 && waitpid(sc->pid, &status, 0) == sc->pid) {
        if (WIFEXITED(status)) snprintf(detail, sizeof(detail), "The WhatsApp bridge exited (code %d).", WEXITSTATUS(status));
        else if (WIFSIGNALED(status)) snprintf(detail, sizeof(detail), "The WhatsApp bridge was killed (signal %d).", WTERMSIG(status));
    }
    sc->pid = -1;
    push_simple(sc, EVENT_SIDECAR_EXITED, detail);
    return NULL;
}

static int send_line(Sidecar *sc, char *json) {
    if (!json) return -1;
    int rc = -1;
    pthread_mutex_lock(&sc->write_lock);
    if (sc->to_child >= 0) {
        size_t len = strlen(json);
        json[len] = '\0';
        ssize_t a = write(sc->to_child, json, len);
        ssize_t b = write(sc->to_child, "\n", 1);
        rc = (a == (ssize_t)len && b == 1) ? 0 : -1;
    }
    pthread_mutex_unlock(&sc->write_lock);
    free(json);
    return rc;
}

static void close_fd(int *fd) {
    if (*fd >= 0) close(*fd);
    *fd = -1;
}

static void gw_stop(IMessageGateway *self) {
    Sidecar *sc = ctx_of(self);
    pthread_mutex_lock(&sc->write_lock);
    close_fd(&sc->to_child);
    pthread_mutex_unlock(&sc->write_lock);
    pid_t pid = sc->pid;
    if (pid > 0) {
        kill(pid, SIGTERM);
        for (int i = 0; i < 20 && sc->pid > 0; i++) {
            struct timespec ts = { 0, 100 * 1000000L };
            nanosleep(&ts, NULL);
        }
        if (sc->pid > 0) kill(pid, SIGKILL);
    }
    if (sc->reader_running) {
        pthread_join(sc->reader, NULL);
        sc->reader_running = 0;
    }
    close_fd(&sc->from_child);
}

/* pipe2() is missing on macOS before 27; there, set close-on-exec by hand. */
static int pipe_cloexec(int fds[2]) {
#ifndef __APPLE__
    return pipe2(fds, O_CLOEXEC);
#else
    if (pipe(fds) != 0) return -1;
    if (fcntl(fds[0], F_SETFD, FD_CLOEXEC) != 0 || fcntl(fds[1], F_SETFD, FD_CLOEXEC) != 0) {
        close(fds[0]); close(fds[1]);
        return -1;
    }
    return 0;
#endif
}

static int gw_start(IMessageGateway *self) {
    Sidecar *sc = ctx_of(self);
    if (sc->reader_running) gw_stop(self);

    char entry[1024];
    path_join(entry, sizeof(entry), sc->options.sidecar_dir, "src/index.js");
    if (!path_is_regular_file(entry)) {
        push_simple(sc, EVENT_SIDECAR_EXITED, "The Node.js bridge is not installed (sidecar folder missing).");
        return -1;
    }

    int in_pipe[2], out_pipe[2];
    if (pipe_cloexec(in_pipe) != 0) return -1;
    if (pipe_cloexec(out_pipe) != 0) { close(in_pipe[0]); close(in_pipe[1]); return -1; }

    pid_t pid = fork();
    if (pid < 0) {
        close(in_pipe[0]); close(in_pipe[1]); close(out_pipe[0]); close(out_pipe[1]);
        return -1;
    }
    if (pid == 0) {
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);
        int err = open(sc->options.log_path[0] ? sc->options.log_path : "/dev/null",
                       O_WRONLY | O_CREAT | O_APPEND | O_NOFOLLOW, 0600);
        if (err >= 0) dup2(err, STDERR_FILENO);
        setpgid(0, 0);
        char *const argv[] = {
            sc->options.node_binary, entry,
            "--auth-dir", sc->options.auth_dir,
            "--media-dir", sc->options.media_dir,
            sc->options.debug ? "--debug" : NULL, NULL
        };
        execvp(argv[0], argv);
        _exit(127);
    }
    close(in_pipe[0]);
    close(out_pipe[1]);
    sc->pid = pid;
    sc->to_child = in_pipe[1];
    sc->from_child = out_pipe[0];
    if (pthread_create(&sc->reader, NULL, reader_main, sc) != 0) {
        gw_stop(self);
        return -1;
    }
    sc->reader_running = 1;
    LOG_INFO("sidecar started (pid %d)", (int)pid);
    return 0;
}

static int gw_connect(IMessageGateway *self) { return send_line(ctx_of(self), json_protocol_encode_connect()); }
static int gw_reconnect(IMessageGateway *self) { return send_line(ctx_of(self), json_protocol_encode_reconnect()); }
static int gw_send_text(IMessageGateway *self, const char *jid, const OutgoingText *text, const char *id) {
    return send_line(ctx_of(self), json_protocol_encode_send(jid, text, id));
}
static int gw_reject_call(IMessageGateway *self, const char *from, const char *call_id) {
    return send_line(ctx_of(self), json_protocol_encode_reject_call(from, call_id));
}

static int gw_request_profile(IMessageGateway *self, const char *jid) {
    return send_line(ctx_of(self), json_protocol_encode_profile(jid));
}

static int gw_request_picture(IMessageGateway *self, const char *jid, int full) {
    return send_line(ctx_of(self), json_protocol_encode_picture(jid, full));
}

static int gw_set_blocked(IMessageGateway *self, const char *jid, int blocked) {
    return send_line(ctx_of(self), json_protocol_encode_block(jid, blocked));
}

static int gw_delete_chat(IMessageGateway *self, const DeleteRequest *r) {
    return send_line(ctx_of(self), json_protocol_encode_delete_chat(r));
}

static int gw_delete(IMessageGateway *self, const DeleteRequest *r) {
    return send_line(ctx_of(self), json_protocol_encode_delete(r));
}

static int gw_edit(IMessageGateway *self, const char *jid, const char *id, const char *text) {
    return send_line(ctx_of(self), json_protocol_encode_edit(jid, id, text));
}
static int gw_react(IMessageGateway *self, const ReactionTarget *t, const char *emoji) {
    return send_line(ctx_of(self), json_protocol_encode_react(t, emoji));
}
static int gw_typing(IMessageGateway *self, const char *jid, const char *state) {
    return send_line(ctx_of(self), json_protocol_encode_typing(jid, state));
}
static int gw_subscribe(IMessageGateway *self, const char *jid) {
    return send_line(ctx_of(self), json_protocol_encode_subscribe(jid));
}
static int gw_request_older(IMessageGateway *self, const HistoryAnchor *a, int count) {
    return send_line(ctx_of(self), json_protocol_encode_history(a, count));
}
static int gw_presence(IMessageGateway *self, int available) {
    return send_line(ctx_of(self), json_protocol_encode_presence(available));
}
static int gw_send_voice(IMessageGateway *self, const char *jid, const char *path, int seconds, const char *id) {
    return send_line(ctx_of(self), json_protocol_encode_send_voice(jid, path, seconds, id));
}
static int gw_send_media(IMessageGateway *self, const char *jid, const OutgoingMedia *media, const char *id) {
    return send_line(ctx_of(self), json_protocol_encode_send_media(jid, media, id));
}

static int gw_forward_media(IMessageGateway *self, const char *jid, const char *ref, int score, const char *id) {
    return send_line(ctx_of(self), json_protocol_encode_forward_media(jid, ref, score, id));
}
static int gw_pair(IMessageGateway *self, const char *phone) { return send_line(ctx_of(self), json_protocol_encode_pair(phone)); }
static int gw_qr(IMessageGateway *self) { return send_line(ctx_of(self), json_protocol_encode_qr()); }
static int gw_download(IMessageGateway *self, const char *id, const char *ref, int max_mb) {
    return send_line(ctx_of(self), json_protocol_encode_download(id, ref, max_mb));
}
static int gw_read(IMessageGateway *self, const ReadRequest *r) { return send_line(ctx_of(self), json_protocol_encode_read(r)); }
static int gw_logout(IMessageGateway *self) { return send_line(ctx_of(self), json_protocol_encode_logout()); }

static int ed_set_name(IProfileEditor *self, const char *name) {
    return send_line(self->ctx, json_protocol_encode_set_name(name));
}
static int ed_set_about(IProfileEditor *self, const char *text) {
    return send_line(self->ctx, json_protocol_encode_set_about(text));
}
static int ed_set_picture(IProfileEditor *self, const char *path) {
    return send_line(self->ctx, json_protocol_encode_set_picture(path));
}
static int ed_remove_picture(IProfileEditor *self) { return send_line(self->ctx, json_protocol_encode_remove_picture()); }

static int lk_like(IStatusLiker *self, const char *author, const char *status_id, const char *emoji) {
    return send_line(self->ctx, json_protocol_encode_like_status(author, status_id, emoji));
}

static void gw_destroy(IMessageGateway *self) {
    if (!self) return;
    gw_stop(self);
    pthread_mutex_destroy(&ctx_of(self)->write_lock);
    free(self->ctx);
    free(self);
}

IMessageGateway *sidecar_gateway_create(const GatewayOptions *options, EventQueue *events) {
    IMessageGateway *gw = calloc(1, sizeof(*gw));
    Sidecar *sc = calloc(1, sizeof(*sc));
    if (!gw || !sc) { free(gw); free(sc); return NULL; }
    sc->options = *options;
    sc->events = events;
    sc->pid = -1;
    sc->to_child = -1;
    sc->from_child = -1;
    pthread_mutex_init(&sc->write_lock, NULL);
    sc->editor = (IProfileEditor){ sc, ed_set_name, ed_set_about, ed_set_picture, ed_remove_picture };
    sc->liker = (IStatusLiker){ sc, lk_like };
    gw->ctx = sc;
    gw->start = gw_start;
    gw->connect = gw_connect;
    gw->reconnect = gw_reconnect;
    gw->stop = gw_stop;
    gw->send_text = gw_send_text;
    gw->react = gw_react;
    gw->edit = gw_edit;
    gw->delete_message = gw_delete;
    gw->delete_chat = gw_delete_chat;
    gw->request_profile = gw_request_profile;
    gw->request_picture = gw_request_picture;
    gw->set_blocked = gw_set_blocked;
    gw->reject_call = gw_reject_call;
    gw->typing = gw_typing;
    gw->subscribe = gw_subscribe;
    gw->presence = gw_presence;
    gw->request_older = gw_request_older;
    gw->send_voice = gw_send_voice;
    gw->send_media = gw_send_media;
    gw->forward_media = gw_forward_media;
    gw->request_pairing_code = gw_pair;
    gw->request_qr = gw_qr;
    gw->download_media = gw_download;
    gw->mark_read = gw_read;
    gw->logout = gw_logout;
    gw->destroy = gw_destroy;
    return gw;
}

IStatusLiker *sidecar_gateway_status_liker(IMessageGateway *gateway) {
    return gateway ? &ctx_of(gateway)->liker : NULL;
}

IProfileEditor *sidecar_gateway_profile_editor(IMessageGateway *gateway) {
    return gateway ? &ctx_of(gateway)->editor : NULL;
}
