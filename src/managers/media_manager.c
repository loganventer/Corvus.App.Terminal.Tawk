#include "managers/media_manager.h"
#include "utilities/file_copy.h"
#include "engines/message_id_generator.h"
#include "utilities/path_util.h"
#include "utilities/process_util.h"
#include "utilities/clock_util.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MediaManager {
    MediaManagerDeps deps;
    char             recording_path[512];
    char             video_path[512];
    int64_t          video_started_ms;
};

MediaManager *media_manager_create(const MediaManagerDeps *deps) {
    MediaManager *m = calloc(1, sizeof(*m));
    if (m) m->deps = *deps;
    return m;
}

void media_manager_destroy(MediaManager *m) {
    if (!m) return;
    if (m->deps.recorder->is_recording(m->deps.recorder)) m->deps.recorder->cancel(m->deps.recorder);
    media_manager_stop_camera(m);
    free(m);
}

int media_manager_activate(MediaManager *m, const char *path, MessageType type) {
    if (!path || !path_is_regular_file(path)) return -1;
    if (type == MESSAGE_TYPE_AUDIO) {
        IAudioPlayer *p = m->deps.voice_player;
        if (p->is_playing(p) && strcmp(p->current(p), path) == 0) {
            p->stop(p);
            return 0;
        }
        return p->play(p, path);
    }
    return m->deps.opener->open(m->deps.opener, path, type);
}

void media_manager_stop_playback(MediaManager *m) {
    m->deps.voice_player->stop(m->deps.voice_player);
}

const char *media_manager_playing(MediaManager *m) {
    return m->deps.voice_player->current(m->deps.voice_player);
}

int64_t media_manager_playing_ms(MediaManager *m) {
    return m->deps.voice_player->elapsed_ms(m->deps.voice_player);
}

/* A new file in the outgoing media folder, named by a fresh message id. */
static int outgoing_path(MediaManager *m, const char *extension, char *out, size_t size) {
    char dir[512], id[32], name[64];
    path_join(dir, sizeof(dir), m->deps.settings->media_dir, "outgoing");
    if (path_mkdir_p(dir, 0700) != 0 || message_id_generate(id, sizeof(id)) != 0) return -1;
    snprintf(name, sizeof(name), "%s.%s", id, extension);
    path_join(out, size, dir, name);
    return 0;
}

int media_manager_camera_available(MediaManager *m) {
    return m->deps.camera && m->deps.camera->available(m->deps.camera);
}

/* Photos are sent at this size (16:9, like laptop cameras); the preview uses the same frames. */
#define PHOTO_WIDTH  1280
#define PHOTO_HEIGHT 720

int media_manager_start_camera(MediaManager *m) {
    if (!media_manager_camera_available(m)) return -1;
    return m->deps.camera->start_preview(m->deps.camera, PHOTO_WIDTH, PHOTO_HEIGHT);
}

int media_manager_camera_frame(MediaManager *m, RgbImage *frame) {
    return m->deps.camera && m->deps.camera->latest_frame(m->deps.camera, frame);
}

CameraResult media_manager_snap_photo(MediaManager *m, char *out, size_t size) {
    if (!m->deps.camera) return CAMERA_NOT_FOUND;
    if (outgoing_path(m, "jpg", out, size) != 0) return CAMERA_FAILED;
    return m->deps.camera->snap(m->deps.camera, out);
}

void media_manager_stop_camera(MediaManager *m) {
    if (m->deps.camera) m->deps.camera->stop_preview(m->deps.camera);
    m->video_path[0] = '\0';
}

/* Videos stop by themselves after three minutes, keeping them a sensible size to send. */
#define VIDEO_MAX_MS (3 * 60 * 1000)

int media_manager_start_video(MediaManager *m) {
    ICamera *cam = m->deps.camera;
    if (!cam || cam->is_recording(cam)) return -1;
    if (outgoing_path(m, "mp4", m->video_path, sizeof(m->video_path)) != 0) return -1;
    char *argv[PROCESS_MAX_ARGS];
    char storage[1024];
    IAudioBackend *audio = m->deps.audio;
    int sound = audio && audio->available(audio) &&
                audio->capture_argv(audio, m->deps.settings->mic_device, argv, PROCESS_MAX_ARGS, storage, sizeof(storage)) >= 0;
    media_manager_stop_playback(m);
    if (cam->start_recording(cam, m->video_path, sound ? argv : NULL) != 0) { m->video_path[0] = '\0'; return -1; }
    m->video_started_ms = clock_now_ms();
    return 0;
}

int media_manager_finish_video(MediaManager *m, char *path, size_t size, int *seconds) {
    ICamera *cam = m->deps.camera;
    if (!cam || !cam->is_recording(cam) || !m->video_path[0]) return -1;
    int64_t elapsed = clock_now_ms() - m->video_started_ms;
    int rc = cam->stop_recording(cam);
    if (rc == 0) {
        str_copy(path, size, m->video_path);
        if (seconds) *seconds = (int)((elapsed + 500) / 1000);
    }
    m->video_path[0] = '\0';
    return rc;
}

int media_manager_is_recording_video(MediaManager *m) {
    return m->deps.camera && m->deps.camera->is_recording(m->deps.camera);
}

int media_manager_video_seconds(MediaManager *m) {
    return media_manager_is_recording_video(m) ? (int)((clock_now_ms() - m->video_started_ms) / 1000) : 0;
}

int media_manager_video_limit_reached(MediaManager *m) {
    return media_manager_is_recording_video(m) && clock_now_ms() - m->video_started_ms >= VIDEO_MAX_MS;
}

int media_manager_start_recording(MediaManager *m) {
    IAudioRecorder *r = m->deps.recorder;
    if (r->is_recording(r)) return -1;
    if (outgoing_path(m, "ogg", m->recording_path, sizeof(m->recording_path)) != 0) return -1;
    media_manager_stop_playback(m);
    return r->start(r, m->recording_path, m->deps.settings->mic_device);
}

int media_manager_finish_recording(MediaManager *m, char *path, size_t size, int *seconds) {
    IAudioRecorder *r = m->deps.recorder;
    int64_t elapsed = r->elapsed_ms(r);
    if (r->stop(r) != 0 || elapsed < 700) {   /* ignore accidental taps */
        media_manager_cancel_recording(m);
        return -1;
    }
    str_copy(path, size, m->recording_path);
    *seconds = (int)((elapsed + 500) / 1000);
    m->recording_path[0] = '\0';
    return 0;
}

void media_manager_cancel_recording(MediaManager *m) {
    m->deps.recorder->cancel(m->deps.recorder);
    m->recording_path[0] = '\0';
}

int media_manager_is_recording(MediaManager *m) {
    return m->deps.recorder->is_recording(m->deps.recorder);
}

int media_manager_recording_seconds(MediaManager *m) {
    return (int)(m->deps.recorder->elapsed_ms(m->deps.recorder) / 1000);
}

int media_manager_recording_limit_reached(MediaManager *m) {
    return media_manager_is_recording(m) &&
           media_manager_recording_seconds(m) >= m->deps.settings->voice_max_seconds;
}

int media_manager_save_copy(MediaManager *m, const char *path, const char *name, char *out, size_t size) {
    if (!path || !path_is_regular_file(path)) return -1;
    char dir[512];
    if (m->deps.settings->download_dir[0]) path_expand_home(m->deps.settings->download_dir, dir, sizeof(dir));
    else path_download_dir(dir, sizeof(dir));
    if (path_mkdir_p(dir, 0755) != 0) return -1;
    if (file_unique_path(dir, name && name[0] ? name : "file", out, size) != 0) return -1;
    return file_copy(path, out, 0644);
}
