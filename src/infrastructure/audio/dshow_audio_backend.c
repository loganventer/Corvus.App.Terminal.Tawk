#include "infrastructure/audio/dshow_audio_backend.h"
#include "infrastructure/audio/ffplay_fallback.h"
#include "utilities/argv_builder.h"
#include "utilities/process_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *ds_name(IAudioBackend *self) { (void)self; return "dshow"; }

static int ds_available(IAudioBackend *self) {
    (void)self;
#if defined(__CYGWIN__) || defined(__MSYS__) || defined(_WIN32)
    return process_on_path("ffmpeg");
#else
    return 0;
#endif
}

static int ds_capture(IAudioBackend *self, const char *device, char **argv, int max, char *st, size_t size) {
    (void)self;
    /* DirectShow has no "default" alias; the device name must be configured
     * (list them with: ffmpeg -list_devices true -f dshow -i dummy). */
    char input[200];
    snprintf(input, sizeof(input), "audio=%s", (device && *device && strcmp(device, "default") != 0) ? device : "Microphone");
    ArgvBuilder b;
    argv_builder_init(&b, argv, max, st, size);
    const char *args[] = { "ffmpeg", "-hide_banner", "-loglevel", "error", "-nostdin", "-f", "dshow",
                           "-i", input, "-f", "s16le", "-ar", "48000", "-ac", "1", "-", NULL };
    for (int i = 0; args[i]; i++) argv_builder_add(&b, args[i]);
    return argv_builder_finish(&b);
}

static int ds_playback(IAudioBackend *self, const char *path, char **argv, int max, char *st, size_t size) {
    (void)self;
    ArgvBuilder b;
    argv_builder_init(&b, argv, max, st, size);
    if (ffplay_fallback_add(&b, path) != 0) return -1;
    return argv_builder_finish(&b);
}

static void ds_destroy(IAudioBackend *self) { free(self); }

IAudioBackend *dshow_audio_backend_create(void) {
    IAudioBackend *b = calloc(1, sizeof(*b));
    if (!b) return NULL;
    b->name = ds_name;
    b->available = ds_available;
    b->capture_argv = ds_capture;
    b->playback_argv = ds_playback;
    b->destroy = ds_destroy;
    return b;
}
