/* Taking a photo or a video from the + button: the menu, the camera view,
 * the MediaManager use cases with a fake camera, and the ffmpeg camera itself
 * fed by ffmpeg's test pattern and a test tone instead of a real camera. */
#include "clients/tui/attach_menu.h"
#include "clients/tui/camera_view.h"
#include "contracts/i_audio_player.h"
#include "contracts/i_audio_recorder.h"
#include "contracts/i_camera.h"
#include "core/settings.h"
#include "infrastructure/ffmpeg_camera.h"
#include "managers/media_manager.h"

#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

/* ---- a fake camera for the manager ---------------------------------------- */

typedef struct FakeCamera {
    int           present, streaming, frames_left, recording;
    char          video[1024];
    unsigned char pixels[4 * 2 * 3];
    char          snapped[1024];
} FakeCamera;

static FakeCamera *fake_of(ICamera *self) { return self->ctx; }
static int  fake_available(ICamera *self) { return fake_of(self)->present; }
static int  fake_start(ICamera *self, int w, int h) { (void)w; (void)h; fake_of(self)->streaming = 1; return 0; }
static void fake_stop(ICamera *self) { fake_of(self)->streaming = 0; }

static int fake_frame(ICamera *self, RgbImage *frame) {
    FakeCamera *f = fake_of(self);
    if (!f->streaming || f->frames_left <= 0) return 0;
    f->frames_left--;
    *frame = (RgbImage){ 4, 2, f->pixels };
    return 1;
}

static CameraResult fake_snap(ICamera *self, const char *path) {
    snprintf(fake_of(self)->snapped, sizeof(fake_of(self)->snapped), "%s", path);
    FILE *out = fopen(path, "wb");
    if (!out) return CAMERA_FAILED;
    fputs("jpeg", out);
    fclose(out);
    return CAMERA_SAVED;
}

static int fake_start_recording(ICamera *self, const char *path, char *const audio[]) {
    (void)audio;
    FakeCamera *f = fake_of(self);
    snprintf(f->video, sizeof(f->video), "%s", path);
    f->recording = 1;
    return 0;
}

static int fake_stop_recording(ICamera *self) {
    FakeCamera *f = fake_of(self);
    FILE *out = fopen(f->video, "wb");
    if (out) { fputs("....ftyp", out); fclose(out); }
    f->recording = f->streaming = 0;
    return out ? 0 : -1;
}

static int fake_is_recording(ICamera *self) { return fake_of(self)->recording; }

/* MediaManager only asks the recorder whether it is recording, when destroyed. */
static int fake_not_recording(IAudioRecorder *self) { (void)self; return 0; }
/* Starting a video stops any voice note playing. */
static void fake_stop_playing(IAudioPlayer *self) { (void)self; }

static void test_menu(void) {
    AttachMenu m;
    attach_menu_open(&m, 1);
    CHECK(m.open && attach_menu_choice(&m) == ATTACH_CHOICE_PHOTO, "with a camera the menu starts on Take a photo");
    attach_menu_key(&m, 1, KEY_DOWN);
    CHECK(attach_menu_choice(&m) == ATTACH_CHOICE_FILE, "Down moves to Choose a file");
    CHECK(attach_menu_key(&m, 0, '\n') == POPUP_CHOSEN && !m.open, "Enter chooses and closes");

    attach_menu_open(&m, 0);
    CHECK(attach_menu_choice(&m) == ATTACH_CHOICE_FILE, "without a camera the menu starts on Choose a file");
    CHECK(attach_menu_key(&m, 0, 27) == POPUP_CLOSED && !m.open, "Esc closes");

    attach_menu_open(&m, 1);
    m.last_rect = (UiRect){ 10, 20, ATTACH_CHOICE_COUNT + 2, 34 };      /* as rendered */
    CHECK(attach_menu_click(&m, 11, 25) == POPUP_CHOSEN && attach_menu_choice(&m) == ATTACH_CHOICE_PHOTO,
          "clicking the first row takes a photo");
    attach_menu_open(&m, 1);
    m.last_rect = (UiRect){ 10, 20, ATTACH_CHOICE_COUNT + 2, 34 };
    CHECK(attach_menu_click(&m, 12, 25) == POPUP_CHOSEN && attach_menu_choice(&m) == ATTACH_CHOICE_FILE,
          "clicking the second row chooses a file");
    attach_menu_open(&m, 1);
    m.last_rect = (UiRect){ 10, 20, ATTACH_CHOICE_COUNT + 2, 34 };
    CHECK(attach_menu_click(&m, 2, 2) == POPUP_CLOSED && !m.open, "clicking outside closes");
}

static void test_camera_view(void) {
    CameraView v;
    memset(&v, 0, sizeof(v));
    camera_view_open(&v);
    CHECK(v.open && v.phase == CAMERA_VIEW_LIVE, "the view opens on the live picture");
    CHECK(camera_view_key(&v, 0, ' ') == CAMERA_VIEW_SNAP, "Space takes the photo");
    CHECK(camera_view_key(&v, 0, '\n') == CAMERA_VIEW_SNAP, "Enter takes the photo");
    CHECK(camera_view_key(&v, 0, 27) == CAMERA_VIEW_CANCEL, "Esc cancels");
    CHECK(camera_view_key(&v, 0, 'r') == CAMERA_VIEW_NONE, "R does nothing while aiming");

    unsigned char a[4 * 2 * 3], b[4 * 2 * 3];
    memset(a, 10, sizeof(a));
    memset(b, 200, sizeof(b));
    RgbImage fa = { 4, 2, a }, fb = { 4, 2, b };
    camera_view_set_frame(&v, &fa);
    CHECK(v.frame.pixels && v.frame.pixels != a && v.frame.pixels[0] == 10, "a live frame is copied");

    camera_view_review(&v, "/tmp/photo.jpg");
    camera_view_set_frame(&v, &fb);
    CHECK(v.phase == CAMERA_VIEW_REVIEW && v.frame.pixels[0] == 10, "the picture freezes while reviewing");
    CHECK(camera_view_key(&v, 0, '\n') == CAMERA_VIEW_USE, "Enter uses the photo");
    CHECK(camera_view_key(&v, 0, 'r') == CAMERA_VIEW_RETAKE, "R retakes");
    CHECK(camera_view_key(&v, 0, 27) == CAMERA_VIEW_CANCEL, "Esc cancels while reviewing");

    camera_view_live(&v);
    camera_view_set_frame(&v, &fb);
    CHECK(v.phase == CAMERA_VIEW_LIVE && v.frame.pixels[0] == 200 && !v.photo[0], "a retake goes back to the live picture");

    CHECK(camera_view_key(&v, 0, 'v') == CAMERA_VIEW_START_VIDEO, "V starts a video");
    camera_view_recording(&v);
    camera_view_set_frame(&v, &fa);
    CHECK(v.phase == CAMERA_VIEW_RECORDING && v.frame.pixels[0] == 10, "the picture stays live while recording");
    CHECK(camera_view_key(&v, 0, 'v') == CAMERA_VIEW_STOP_VIDEO && camera_view_key(&v, 0, ' ') == CAMERA_VIEW_STOP_VIDEO,
          "V or Space stops the video");
    CHECK(camera_view_key(&v, 0, 27) == CAMERA_VIEW_CANCEL, "Esc discards the video");
    camera_view_review_video(&v, "/tmp/video.mp4", 12);
    CHECK(v.phase == CAMERA_VIEW_REVIEW && v.video && v.seconds == 12, "a video is reviewed with its length");
    CHECK(camera_view_key(&v, 0, 'p') == CAMERA_VIEW_PLAY && camera_view_key(&v, 0, '\n') == CAMERA_VIEW_USE,
          "P plays the video, Enter uses it");
    camera_view_live(&v);
    CHECK(!v.video && camera_view_key(&v, 0, 'p') == CAMERA_VIEW_NONE, "a retake is back to photos and videos");
    camera_view_close(&v);
    CHECK(!v.open && !v.frame.pixels, "closing frees the picture");
}

static void test_manager(const char *dir) {
    Settings s;
    settings_set_defaults(&s);
    snprintf(s.media_dir, sizeof(s.media_dir), "%s", dir);
    FakeCamera fake = { .present = 1, .frames_left = 2 };
    ICamera camera = { .ctx = &fake, .available = fake_available, .start_preview = fake_start, .latest_frame = fake_frame,
                       .snap = fake_snap, .stop_preview = fake_stop, .start_recording = fake_start_recording,
                       .stop_recording = fake_stop_recording, .is_recording = fake_is_recording };
    IAudioRecorder recorder = { .is_recording = fake_not_recording };
    IAudioPlayer player = { .stop = fake_stop_playing };
    MediaManagerDeps deps = { .voice_player = &player, .recorder = &recorder, .camera = &camera, .settings = &s };
    MediaManager *m = media_manager_create(&deps);

    RgbImage frame;
    char path[1024] = "", outgoing[600];
    CHECK(media_manager_camera_available(m), "the camera is available");
    CHECK(media_manager_start_camera(m) == 0 && fake.streaming, "the live picture starts");
    CHECK(media_manager_camera_frame(m, &frame) == 1 && frame.width == 4, "frames come through");
    CHECK(media_manager_snap_photo(m, path, sizeof(path)) == CAMERA_SAVED, "a photo is snapped");
    snprintf(outgoing, sizeof(outgoing), "%s/outgoing/", dir);
    CHECK(strncmp(path, outgoing, strlen(outgoing)) == 0 && strstr(path, ".jpg") && strcmp(path, fake.snapped) == 0,
          "it is saved as a .jpg in the outgoing folder");
    unlink(path);
    media_manager_stop_camera(m);
    CHECK(!fake.streaming, "the live picture stops");

    int seconds = -1;
    CHECK(media_manager_start_camera(m) == 0 && media_manager_start_video(m) == 0 && media_manager_is_recording_video(m),
          "a video starts");
    CHECK(media_manager_finish_video(m, path, sizeof(path), &seconds) == 0 && seconds == 0 && strstr(path, ".mp4") &&
          strncmp(path, outgoing, strlen(outgoing)) == 0 && !media_manager_is_recording_video(m),
          "it finishes as an .mp4 in the outgoing folder");
    unlink(path);

    fake.present = 0;
    CHECK(!media_manager_camera_available(m) && media_manager_start_camera(m) != 0, "without a camera nothing starts");
    media_manager_destroy(m);
}

static double now_s(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/* The real ffmpeg camera, with ffmpeg's test pattern standing in for a camera. */
static void test_ffmpeg_camera(const char *dir) {
    ICamera *cam = ffmpeg_camera_create_for("lavfi", "testsrc=size=640x480:rate=30");
    CHECK(cam && cam->available(cam), "ffmpeg is there to drive the camera");
    if (!cam || !cam->available(cam)) return;
    CHECK(cam->start_preview(cam, 320, 180) == 0, "the preview starts");
    RgbImage frame = { 0, 0, NULL };
    int frames = 0;
    for (double end = now_s() + 10; now_s() < end && frames < 3;) {
        if (cam->latest_frame(cam, &frame)) frames++;
        else usleep(20000);
    }
    CHECK(frames >= 3, "live frames keep arriving");
    CHECK(frame.width == 320 && frame.height == 180 && frame.pixels, "frames have the size asked for");

    char path[1024];
    snprintf(path, sizeof(path), "%s/snap.jpg", dir);
    CHECK(cam->snap(cam, path) == CAMERA_SAVED, "a frame is snapped");
    FILE *f = fopen(path, "rb");
    unsigned char magic[2] = { 0, 0 };
    if (f) { if (fread(magic, 1, 2, f) != 2) magic[0] = 0; fclose(f); }
    CHECK(magic[0] == 0xFF && magic[1] == 0xD8, "the snapped photo is a JPEG");
    unlink(path);

    double t = now_s();
    cam->stop_preview(cam);
    CHECK(now_s() - t < 3, "the preview stops promptly");
    CHECK(!cam->latest_frame(cam, &frame), "no frames after stopping");

    /* A video with sound: a test tone stands in for the microphone. Recorded
     * with a decimal-comma locale where there is one, as tawk runs in the
     * user's locale and ffmpeg refuses "0,842" for a time. */
    const char *comma[] = { "de_DE.UTF-8", "af_ZA.UTF-8", "nl_NL.UTF-8", "fr_FR.UTF-8", NULL };
    for (int i = 0; comma[i] && !setlocale(LC_NUMERIC, comma[i]); i++) {}
    char probe_number[16];
    snprintf(probe_number, sizeof(probe_number), "%.1f", 0.5);
    printf("  recording with %s decimals\n", strchr(probe_number, ',') ? "comma" : "point");
    char *const tone[] = { "ffmpeg", "-hide_banner", "-loglevel", "error", "-nostdin", "-re", "-f", "lavfi",
                           "-i", "sine=frequency=440:sample_rate=48000", "-ac", "1", "-f", "s16le", "-", NULL };
    char video[1024];
    snprintf(video, sizeof(video), "%s/video.mp4", dir);
    CHECK(cam->start_preview(cam, 320, 180) == 0, "the preview starts again");
    for (double end = now_s() + 10; now_s() < end && !cam->latest_frame(cam, &frame);) usleep(20000);
    CHECK(cam->start_recording(cam, video, tone) == 0 && cam->is_recording(cam), "a recording starts");
    frames = 0;
    for (double end = now_s() + 2.5; now_s() < end;) {
        if (cam->latest_frame(cam, &frame)) frames++;
        else usleep(20000);
    }
    CHECK(frames >= 10, "the preview carries on while recording");
    CHECK(cam->stop_recording(cam) == 0 && !cam->is_recording(cam), "the recording finishes");
    char cmd[1200], probe[256] = "";
    snprintf(cmd, sizeof(cmd), "ffprobe -v error -show_entries stream=codec_name:format=duration -of csv=p=0 '%s' 2>&1", video);
    FILE *pp = popen(cmd, "r");
    if (pp) { size_t got = fread(probe, 1, sizeof(probe) - 1, pp); probe[got] = '\0'; pclose(pp); }
    double duration = 0;
    const char *last = strrchr(probe, '\n');
    while (last && last > probe && last[-1] != '\n') last--;
    for (const char *line = probe; line && *line; line = strchr(line, '\n') ? strchr(line, '\n') + 1 : NULL) {
        double d = atof(line);
        if (d > duration) duration = d;
    }
    CHECK(strstr(probe, "h264") && strstr(probe, "aac"), "the video has H.264 picture and AAC sound");
    CHECK(duration > 1.5 && duration < 5, "it is about as long as it was recorded");
    if (!(strstr(probe, "h264") && strstr(probe, "aac") && duration > 1.5 && duration < 5)) fprintf(stderr, "ffprobe: %s\n", probe);
    unlink(video);
    setlocale(LC_NUMERIC, "C");
    cam->destroy(cam);
}

int main(void) {
    char dir[] = "/tmp/tawk-photo-test-XXXXXX";
    if (!mkdtemp(dir)) { perror("mkdtemp"); return 1; }
    test_menu();
    test_camera_view();
    test_manager(dir);
    test_ffmpeg_camera(dir);
    char outgoing[600];
    snprintf(outgoing, sizeof(outgoing), "%s/outgoing", dir);
    rmdir(outgoing);
    rmdir(dir);
    if (failures) return 1;
    printf("ok: + takes a photo or records a video with a live preview and a review, or picks a file\n");
    return 0;
}
