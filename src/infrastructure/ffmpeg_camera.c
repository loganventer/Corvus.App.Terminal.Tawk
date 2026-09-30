#include "infrastructure/ffmpeg_camera.h"
#include "utilities/child_process.h"
#include "utilities/clock_util.h"
#include "utilities/log.h"
#include "utilities/platform.h"
#include "utilities/process_capture.h"
#include "utilities/process_util.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define LINUX_CAMERA  "/dev/video0"
#define PREVIEW_FPS   "15"
#define ENCODE_TIMEOUT_MS 10000

/* ffmpeg streams raw RGB frames into a pipe; a reader thread keeps only the
 * newest one, so the picture never lags behind however slowly it is shown.
 * While recording, the same ffmpeg also writes the picture to an MP4, and
 * the audio backend's capture command records the sound beside it; when the
 * recording stops the two are joined, the sound trimmed to start with the
 * first frame, so they stay in sync. */
typedef struct Camera {
    char            input_format[16];   /* avfoundation, v4l2, or lavfi in tests */
    char            input[128];
    ChildProcess    ffmpeg;
    ChildProcess    audio;          /* the sound for a recording */
    char            recording[1024]; /* the MP4 being made; "" when not recording */
    int64_t         sound_started_ms;   /* when the sound capture started (0: no sound) */
    int64_t         first_frame_ms;     /* when the first frame of this ffmpeg arrived */
    int             fd;             /* read end of ffmpeg's output */
    pthread_t       reader;
    int             reading;
    pthread_mutex_t lock;
    int             width, height;
    size_t          frame_bytes;
    unsigned char  *filling;        /* the frame being read (reader thread only) */
    unsigned char  *newest;         /* the last complete frame (under lock) */
    int             have_frame;
    int             fresh;          /* newest not handed out yet */
    unsigned char  *handed;         /* the copy latest_frame hands out */
} Camera;

static Camera *ctx_of(ICamera *self) { return self->ctx; }

static int cam_available(ICamera *self) {
    Camera *c = ctx_of(self);
    if (!process_on_path("ffmpeg")) return 0;
    if (strcmp(c->input_format, "v4l2") == 0) return access(c->input, R_OK) == 0;   /* WSL usually has no camera */
    return 1;
}

static void *read_frames(void *arg) {
    Camera *c = arg;
    size_t filled = 0;
    for (;;) {
        ssize_t n = read(c->fd, c->filling + filled, c->frame_bytes - filled);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;                         /* ffmpeg stopped */
        filled += (size_t)n;
        if (filled < c->frame_bytes) continue;
        pthread_mutex_lock(&c->lock);
        if (!c->first_frame_ms) c->first_frame_ms = clock_now_ms();
        unsigned char *done = c->filling;
        c->filling = c->newest;
        c->newest = done;
        c->have_frame = c->fresh = 1;
        pthread_mutex_unlock(&c->lock);
        filled = 0;
    }
    return NULL;
}

/* Stops ffmpeg, the sound and the reader, keeping the frame buffers. With
 * finish set, ffmpeg gets SIGINT and time to close the MP4 properly. */
static void halt(Camera *c, int finish) {
    if (c->audio.pid > 0) child_process_stop(&c->audio, SIGINT, 1000);          /* EOF on ffmpeg's sound */
    if (c->ffmpeg.pid > 0) child_process_stop(&c->ffmpeg, finish ? SIGINT : SIGTERM, finish ? 10000 : 2000);
    if (c->reading) { pthread_join(c->reader, NULL); c->reading = 0; }        /* ends at EOF */
    if (c->fd >= 0) { close(c->fd); c->fd = -1; }
}

/* The temporary files a recording is made of. */
static void part_path(const Camera *c, const char *suffix, char *out, size_t size) {
    snprintf(out, size, "%s.%s", c->recording, suffix);
}

/* ffmpeg's own report of a recording, kept when it fails so the reason can be
 * read; FFREPORT is set only while the process starts. */
static void report_on(const Camera *c) {
    char log[1200], value[1300];
    part_path(c, "ffmpeg.log", log, sizeof(log));
    snprintf(value, sizeof(value), "file=%s:level=32", log);
    setenv("FFREPORT", value, 1);
}

static void report_off(void) { unsetenv("FFREPORT"); }

static void cam_stop_preview(ICamera *self) {
    Camera *c = ctx_of(self);
    halt(c, 0);
    if (c->recording[0]) {                                  /* abandoned */
        char part[1100];
        part_path(c, "picture.mp4", part, sizeof(part)); unlink(part);
        part_path(c, "sound.raw", part, sizeof(part)); unlink(part);
        c->recording[0] = '\0';
    }
    free(c->filling); free(c->newest); free(c->handed);
    c->filling = c->newest = c->handed = NULL;
    c->have_frame = c->fresh = 0;
}

/* Fit the camera's picture into width x height, keeping its shape. */
static void fit_filter(const Camera *c, const char *fps, char *out, size_t size) {
    snprintf(out, size, "scale=%d:%d:force_original_aspect_ratio=decrease,pad=%d:%d:(ow-iw)/2:(oh-ih)/2,fps=%s",
             c->width, c->height, c->width, c->height, fps);
}

/* Starts ffmpeg: the preview alone, or also the picture as an MP4 at `path`. */
static int launch(Camera *c, const char *path) {
    char preview[200], record[200], graph[480];   /* room for both filters and the joins */
    fit_filter(c, PREVIEW_FPS, preview, sizeof(preview));
    fit_filter(c, "30", record, sizeof(record));
    snprintf(graph, sizeof(graph), "[0:v]split=2[a][b];[a]%s[preview];[b]%s,format=yuv420p[video]", preview, record);

    char *argv[48];
    int n = 0;
#define ARG(x) do { if (n < 47) argv[n++] = (char *)(x); } while (0)
    ARG("ffmpeg"); ARG("-hide_banner"); ARG("-loglevel"); ARG("error"); ARG("-nostdin");
    /* No input buffering: each frame goes out as soon as the camera delivers it. */
    ARG("-fflags"); ARG("nobuffer"); ARG("-flags"); ARG("low_delay");
    if (strcmp(c->input_format, "lavfi") == 0) ARG("-re");         /* a test pattern, at camera speed */
    ARG("-f"); ARG(c->input_format);
    if (strcmp(c->input_format, "avfoundation") == 0) { ARG("-framerate"); ARG("30"); }   /* Mac cameras do 30 */
    ARG("-i"); ARG(c->input);
    if (!path) {
        ARG("-vf"); ARG(preview); ARG("-pix_fmt"); ARG("rgb24"); ARG("-f"); ARG("rawvideo"); ARG("-");
    } else {
        ARG("-filter_complex"); ARG(graph);
        ARG("-map"); ARG("[preview]"); ARG("-pix_fmt"); ARG("rgb24"); ARG("-f"); ARG("rawvideo"); ARG("-");
        ARG("-map"); ARG("[video]"); ARG("-c:v"); ARG("libx264"); ARG("-preset"); ARG("veryfast"); ARG("-crf"); ARG("26");
        ARG("-y"); ARG(path);
    }
    argv[n] = NULL;
#undef ARG

    int fds[2];
    if (pipe(fds) != 0) return -1;
    pthread_mutex_lock(&c->lock);
    c->first_frame_ms = 0;
    pthread_mutex_unlock(&c->lock);
    int started = child_process_start_io(&c->ffmpeg, argv, -1, fds[1]) == 0;
    close(fds[1]);
    c->fd = fds[0];
    if (!started || pthread_create(&c->reader, NULL, read_frames, c) != 0) {
        LOG_WARN("camera: could not start ffmpeg");
        halt(c, 0);
        return -1;
    }
    c->reading = 1;
    return 0;
}

static int cam_start_preview(ICamera *self, int width, int height) {
    Camera *c = ctx_of(self);
    cam_stop_preview(self);
    if (!cam_available(self) || width < 16 || height < 16) return -1;
    c->width = width;
    c->height = height;
    c->frame_bytes = (size_t)width * (size_t)height * 3;
    c->filling = malloc(c->frame_bytes);
    c->newest = malloc(c->frame_bytes);
    c->handed = malloc(c->frame_bytes);
    if (!c->filling || !c->newest || !c->handed || launch(c, NULL) != 0) { cam_stop_preview(self); return -1; }
    return 0;
}

/* The camera restarts with the picture going to a second output while the
 * preview carries on; the sound goes to a file of its own. */
static int cam_start_recording(ICamera *self, const char *path, char *const audio_argv[]) {
    Camera *c = ctx_of(self);
    if (!c->reading || c->recording[0] || !path || !path[0]) return -1;
    halt(c, 0);
    snprintf(c->recording, sizeof(c->recording), "%s", path);
    char picture[1100], sound[1100];
    part_path(c, "picture.mp4", picture, sizeof(picture));
    part_path(c, "sound.raw", sound, sizeof(sound));
    c->sound_started_ms = 0;
    char log[1200];
    part_path(c, "ffmpeg.log", log, sizeof(log));
    unlink(log);
    report_on(c);
    int launched = launch(c, picture);
    report_off();
    if (launched != 0) {
        c->recording[0] = '\0';
        launch(c, NULL);                                     /* back to the preview alone */
        return -1;
    }
    if (audio_argv && audio_argv[0]) {
        int fd = open(sound, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0600);
        if (fd >= 0 && child_process_start_io(&c->audio, audio_argv, -1, fd) == 0) c->sound_started_ms = clock_now_ms();
        else LOG_WARN("camera: recording without sound");
        if (fd >= 0) close(fd);
    }
    return 0;
}

static int cam_is_recording(ICamera *self) { return ctx_of(self)->recording[0] != '\0'; }

static int has_data(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && st.st_size > 0;
}

/* Finishes the recording and stops the camera: the picture and the sound
 * (trimmed to start with the first frame) become one MP4. 0 when it holds a video. */
static int cam_stop_recording(ICamera *self) {
    Camera *c = ctx_of(self);
    if (!c->recording[0]) return -1;
    halt(c, 1);
    char picture[1100], sound[1100], lead[32];
    part_path(c, "picture.mp4", picture, sizeof(picture));
    part_path(c, "sound.raw", sound, sizeof(sound));
    int64_t skip_ms = c->sound_started_ms && c->first_frame_ms > c->sound_started_ms ? c->first_frame_ms - c->sound_started_ms : 0;
    snprintf(lead, sizeof(lead), "%lldms", (long long)skip_ms);   /* whole ms: no decimal comma in any locale */
    int ok = has_data(picture);
    report_on(c);                                           /* appends to the recording's report */
    if (ok && c->sound_started_ms && has_data(sound)) {
        char *const argv[] = { "ffmpeg", "-hide_banner", "-loglevel", "error", "-nostdin",
                               "-i", picture, "-ss", lead, "-f", "s16le", "-ar", "48000", "-ac", "1", "-i", sound,
                               "-map", "0:v", "-map", "1:a", "-c:v", "copy", "-c:a", "aac", "-b:a", "96k", "-shortest",
                               "-movflags", "+faststart", "-y", c->recording, NULL };
        int null_fd = open("/dev/null", O_WRONLY);
        ok = process_run_to_fd(argv, null_fd, ENCODE_TIMEOUT_MS * 3) == 0;
        if (null_fd >= 0) close(null_fd);
    } else if (ok) {
        char *const argv[] = { "ffmpeg", "-hide_banner", "-loglevel", "error", "-nostdin", "-i", picture,
                               "-c", "copy", "-movflags", "+faststart", "-y", c->recording, NULL };
        int null_fd = open("/dev/null", O_WRONLY);
        ok = process_run_to_fd(argv, null_fd, ENCODE_TIMEOUT_MS * 3) == 0;
        if (null_fd >= 0) close(null_fd);
    }
    report_off();
    char log[1200];
    part_path(c, "ffmpeg.log", log, sizeof(log));
    LOG_INFO("camera: recorded picture %s, sound %s, sound trimmed by %s s", has_data(picture) ? "yes" : "no",
             c->sound_started_ms && has_data(sound) ? "yes" : "no", lead);
    unlink(picture);
    unlink(sound);
    unsigned char head[8] = { 0 };
    int fd = ok ? open(c->recording, O_RDONLY) : -1;
    ok = fd >= 0 && read(fd, head, sizeof(head)) == (ssize_t)sizeof(head) && memcmp(head + 4, "ftyp", 4) == 0;
    if (fd >= 0) close(fd);
    if (!ok) {
        unlink(c->recording);
        LOG_ERROR("camera: the video could not be saved; ffmpeg's report is %s", log);
    } else {
        unlink(log);
    }
    c->recording[0] = '\0';
    return ok ? 0 : -1;
}

static int cam_latest_frame(ICamera *self, RgbImage *frame) {
    Camera *c = ctx_of(self);
    if (!c->reading) return 0;
    pthread_mutex_lock(&c->lock);
    int fresh = c->fresh;
    if (fresh) {
        memcpy(c->handed, c->newest, c->frame_bytes);
        c->fresh = 0;
    }
    pthread_mutex_unlock(&c->lock);
    if (!fresh) return 0;
    *frame = (RgbImage){ c->width, c->height, c->handed };
    return 1;
}

/* Writes the frame as a PPM, then has ffmpeg turn it into a JPEG. */
static CameraResult cam_snap(ICamera *self, const char *path) {
    Camera *c = ctx_of(self);
    if (!c->reading) return CAMERA_NOT_FOUND;
    char ppm[1100];
    snprintf(ppm, sizeof(ppm), "%s.ppm", path);
    int fd = open(ppm, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0600);
    if (fd < 0) return CAMERA_FAILED;
    char header[64];
    int header_len = snprintf(header, sizeof(header), "P6\n%d %d\n255\n", c->width, c->height);
    pthread_mutex_lock(&c->lock);
    int ok = c->have_frame && write(fd, header, (size_t)header_len) == header_len &&
             write(fd, c->newest, c->frame_bytes) == (ssize_t)c->frame_bytes;
    pthread_mutex_unlock(&c->lock);
    close(fd);
    if (ok) {
        char *const argv[] = { "ffmpeg", "-hide_banner", "-loglevel", "error", "-nostdin", "-i", ppm,
                               "-q:v", "2", "-y", (char *)path, NULL };
        int null_fd = open("/dev/null", O_WRONLY);
        ok = process_run_to_fd(argv, null_fd, ENCODE_TIMEOUT_MS) == 0;
        if (null_fd >= 0) close(null_fd);
    }
    unlink(ppm);
    struct stat st;
    if (!ok || stat(path, &st) != 0 || st.st_size == 0) { unlink(path); return CAMERA_FAILED; }
    return CAMERA_SAVED;
}

static void cam_destroy(ICamera *self) {
    if (!self) return;
    cam_stop_preview(self);
    pthread_mutex_destroy(&ctx_of(self)->lock);
    free(self->ctx);
    free(self);
}

ICamera *ffmpeg_camera_create(void) {
    return platform_is_macos() ? ffmpeg_camera_create_for("avfoundation", "0")
                               : ffmpeg_camera_create_for("v4l2", LINUX_CAMERA);
}

ICamera *ffmpeg_camera_create_for(const char *input_format, const char *input) {
    ICamera *cam = calloc(1, sizeof(*cam));
    Camera *c = calloc(1, sizeof(*c));
    if (!cam || !c) { free(cam); free(c); return NULL; }
    snprintf(c->input_format, sizeof(c->input_format), "%s", input_format);
    snprintf(c->input, sizeof(c->input), "%s", input);
    c->fd = -1;
    c->ffmpeg.pid = -1;
    c->audio.pid = -1;
    pthread_mutex_init(&c->lock, NULL);
    cam->ctx = c;
    cam->available = cam_available;
    cam->start_preview = cam_start_preview;
    cam->latest_frame = cam_latest_frame;
    cam->snap = cam_snap;
    cam->stop_preview = cam_stop_preview;
    cam->start_recording = cam_start_recording;
    cam->stop_recording = cam_stop_recording;
    cam->is_recording = cam_is_recording;
    cam->destroy = cam_destroy;
    return cam;
}
