#include "infrastructure/process_audio_player.h"
#include "utilities/child_process.h"
#include "utilities/clock_util.h"
#include "utilities/log.h"
#include "utilities/process_util.h"
#include "utilities/str_util.h"

#include <signal.h>
#include <stdlib.h>

typedef struct Player {
    IAudioBackend *backend;
    ChildProcess   child;
    char           path[512];
    int64_t        started_ms;
} Player;

static Player *ctx_of(IAudioPlayer *self) { return (Player *)self->ctx; }

static void player_stop(IAudioPlayer *self) {
    Player *p = ctx_of(self);
    child_process_stop(&p->child, SIGTERM, 500);
    p->path[0] = '\0';
}

static int player_play(IAudioPlayer *self, const char *path) {
    Player *p = ctx_of(self);
    player_stop(self);
    char *argv[PROCESS_MAX_ARGS];
    char storage[1024];
    if (!p->backend || p->backend->playback_argv(p->backend, path, argv, PROCESS_MAX_ARGS, storage, sizeof(storage)) < 0) {
        LOG_WARN("audio backend cannot play %s", path);
        return -1;
    }
    if (child_process_start(&p->child, argv) != 0) return -1;
    str_copy(p->path, sizeof(p->path), path);
    p->started_ms = clock_now_ms();
    return 0;
}

static int player_is_playing(IAudioPlayer *self) {
    Player *p = ctx_of(self);
    int running = child_process_running(&p->child);
    if (!running) p->path[0] = '\0';
    return running;
}

static const char *player_current(IAudioPlayer *self) {
    player_is_playing(self);
    return ctx_of(self)->path;
}

static int64_t player_elapsed_ms(IAudioPlayer *self) {
    Player *p = ctx_of(self);
    return player_is_playing(self) ? clock_now_ms() - p->started_ms : 0;
}

static void player_destroy(IAudioPlayer *self) {
    if (!self) return;
    player_stop(self);
    free(self->ctx);
    free(self);
}

IAudioPlayer *process_audio_player_create(IAudioBackend *backend) {
    IAudioPlayer *ap = calloc(1, sizeof(*ap));
    Player *p = calloc(1, sizeof(*p));
    if (!ap || !p) { free(ap); free(p); return NULL; }
    p->backend = backend;
    p->child.pid = -1;
    ap->ctx = p;
    ap->play = player_play;
    ap->stop = player_stop;
    ap->is_playing = player_is_playing;
    ap->current = player_current;
    ap->elapsed_ms = player_elapsed_ms;
    ap->destroy = player_destroy;
    return ap;
}
