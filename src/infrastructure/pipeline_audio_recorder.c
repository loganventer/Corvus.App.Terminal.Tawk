#include "infrastructure/pipeline_audio_recorder.h"
#include "utilities/child_process.h"
#include "utilities/clock_util.h"
#include "utilities/log.h"
#include "utilities/process_util.h"
#include "utilities/str_util.h"

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct Recorder {
    IAudioBackend *backend;
    ChildProcess   capture;
    ChildProcess   encoder;
    char           path[512];
    int64_t        started_ms;
} Recorder;

static Recorder *ctx_of(IAudioRecorder *self) { return (Recorder *)self->ctx; }

static int rec_start(IAudioRecorder *self, const char *path, const char *device) {
    Recorder *r = ctx_of(self);
    if (r->encoder.pid > 0 || !r->backend) return -1;
    if (!process_on_path("ffmpeg")) {
        LOG_WARN("voice notes need ffmpeg on PATH");
        return -1;
    }
    char *capture_argv[PROCESS_MAX_ARGS];
    char storage[1024];
    if (r->backend->capture_argv(r->backend, device, capture_argv, PROCESS_MAX_ARGS, storage, sizeof(storage)) < 0) return -1;

    char *const encoder_argv[] = {
        "ffmpeg", "-hide_banner", "-loglevel", "error", "-nostdin",
        "-f", "s16le", "-ar", "48000", "-ac", "1", "-i", "pipe:0",
        "-c:a", "libopus", "-b:a", "32k", "-application", "voip", "-y", (char *)path, NULL
    };

    int fds[2];
    if (pipe(fds) != 0) return -1;
    int ok = child_process_start_io(&r->encoder, encoder_argv, fds[0], -1) == 0 &&
             child_process_start_io(&r->capture, capture_argv, -1, fds[1]) == 0;
    close(fds[0]);
    close(fds[1]);
    if (!ok) {
        child_process_stop(&r->capture, SIGKILL, 0);
        child_process_stop(&r->encoder, SIGKILL, 0);
        return -1;
    }
    str_copy(r->path, sizeof(r->path), path);
    r->started_ms = clock_now_ms();
    return 0;
}

static int rec_stop(IAudioRecorder *self) {
    Recorder *r = ctx_of(self);
    if (r->encoder.pid <= 0) return -1;
    /* Stopping the capture closes the pipe; ffmpeg sees EOF and writes the Ogg trailer. */
    child_process_stop(&r->capture, SIGINT, 1000);
    child_process_stop(&r->encoder, SIGTERM, 4000);
    struct stat st;
    return (stat(r->path, &st) == 0 && st.st_size > 200) ? 0 : -1;
}

static void rec_cancel(IAudioRecorder *self) {
    Recorder *r = ctx_of(self);
    child_process_stop(&r->capture, SIGKILL, 0);
    child_process_stop(&r->encoder, SIGKILL, 0);
    if (r->path[0]) unlink(r->path);
    r->path[0] = '\0';
}

static int rec_is_recording(IAudioRecorder *self) {
    Recorder *r = ctx_of(self);
    return child_process_running(&r->capture) && child_process_running(&r->encoder);
}

static int64_t rec_elapsed_ms(IAudioRecorder *self) {
    Recorder *r = ctx_of(self);
    return r->encoder.pid > 0 ? clock_now_ms() - r->started_ms : 0;
}

static void rec_destroy(IAudioRecorder *self) {
    if (!self) return;
    if (ctx_of(self)->encoder.pid > 0) rec_cancel(self);
    free(self->ctx);
    free(self);
}

IAudioRecorder *pipeline_audio_recorder_create(IAudioBackend *backend) {
    IAudioRecorder *ar = calloc(1, sizeof(*ar));
    Recorder *r = calloc(1, sizeof(*r));
    if (!ar || !r) { free(ar); free(r); return NULL; }
    r->backend = backend;
    r->capture.pid = -1;
    r->encoder.pid = -1;
    ar->ctx = r;
    ar->start = rec_start;
    ar->stop = rec_stop;
    ar->cancel = rec_cancel;
    ar->is_recording = rec_is_recording;
    ar->elapsed_ms = rec_elapsed_ms;
    ar->destroy = rec_destroy;
    return ar;
}
