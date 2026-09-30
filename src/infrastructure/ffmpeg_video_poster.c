#include "infrastructure/ffmpeg_video_poster.h"
#include "utilities/clock_util.h"
#include "utilities/file_settled.h"
#include "utilities/process_util.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACKED      64
#define WAIT_MS      15000     /* give up on a frame after this long */

typedef struct Attempt {
    char    path[512];
    int64_t started_ms;
    int     done;
} Attempt;

typedef struct Poster {
    Attempt attempts[TRACKED];
    int     next;
    int     have_ffmpeg;
} Poster;

/* Finished: the tool has stopped writing it (a half-written file would not decode). */
static int ready(const char *path) { return file_settled(path, 300); }

static Attempt *find(Poster *p, const char *video) {
    for (int i = 0; i < TRACKED; i++) if (strcmp(p->attempts[i].path, video) == 0) return &p->attempts[i];
    return NULL;
}

static int get(IVideoPoster *self, const char *video, char *out, size_t size) {
    Poster *p = self->ctx;
    if (!video || !video[0] || strlen(video) + 12 >= size) return 0;
    snprintf(out, size, "%s.poster.jpg", video);
    Attempt *a = find(p, video);
    if (ready(out)) { if (a) a->done = 1; return 1; }
    if (a || !p->have_ffmpeg) return 0;                       /* already tried, or cannot */
    a = &p->attempts[p->next];
    p->next = (p->next + 1) % TRACKED;
    str_copy(a->path, sizeof(a->path), video);
    a->started_ms = clock_now_ms();
    a->done = 0;
    char *const argv[] = { "ffmpeg", "-nostdin", "-loglevel", "error", "-y", "-ss", "0.5", "-i", (char *)video,
                           "-frames:v", "1", "-vf", "scale='min(960,iw)':-2", out, NULL };
    if (process_spawn_detached(argv) != 0) a->done = 1;
    return 0;
}

static int pending(IVideoPoster *self) {
    Poster *p = self->ctx;
    int64_t now = clock_now_ms();
    int n = 0;
    for (int i = 0; i < TRACKED; i++) {
        Attempt *a = &p->attempts[i];
        if (!a->path[0] || a->done) continue;
        if (now - a->started_ms > WAIT_MS) { a->done = 1; continue; }
        n++;                        /* still pending until get() hands the frame to the view */
    }
    return n;
}

static void destroy(IVideoPoster *self) {
    if (!self) return;
    free(self->ctx);
    free(self);
}

IVideoPoster *ffmpeg_video_poster_create(void) {
    IVideoPoster *v = calloc(1, sizeof(*v));
    Poster *p = calloc(1, sizeof(*p));
    if (!v || !p) { free(v); free(p); return NULL; }
    p->have_ffmpeg = process_on_path("ffmpeg");
    v->ctx = p;
    v->get = get;
    v->pending = pending;
    v->destroy = destroy;
    return v;
}
