#include "infrastructure/audio/alsa_audio_backend.h"
#include "infrastructure/audio/ffplay_fallback.h"
#include "utilities/argv_builder.h"
#include "utilities/process_util.h"

#include <stdlib.h>
#include <string.h>

/* Bare ALSA (no sound server). */

static const char *alsa_name(IAudioBackend *self) { (void)self; return "alsa"; }

static int alsa_available(IAudioBackend *self) {
    (void)self;
    return process_on_path("arecord");
}

static int alsa_capture(IAudioBackend *self, const char *device, char **argv, int max, char *st, size_t size) {
    (void)self;
    ArgvBuilder b;
    argv_builder_init(&b, argv, max, st, size);
    argv_builder_add(&b, "arecord");
    argv_builder_add(&b, "-q");
    argv_builder_add(&b, "-f");
    argv_builder_add(&b, "S16_LE");
    argv_builder_add(&b, "-r");
    argv_builder_add(&b, "48000");
    argv_builder_add(&b, "-c");
    argv_builder_add(&b, "1");
    argv_builder_add(&b, "-t");
    argv_builder_add(&b, "raw");
    if (device && *device && strcmp(device, "default") != 0) {
        argv_builder_add(&b, "-D");
        argv_builder_add(&b, device);
    }
    return argv_builder_finish(&b);
}

static int alsa_playback(IAudioBackend *self, const char *path, char **argv, int max, char *st, size_t size) {
    (void)self;
    ArgvBuilder b;
    argv_builder_init(&b, argv, max, st, size);
    if (ffplay_fallback_add(&b, path) != 0) {
        argv_builder_add(&b, "aplay");
        argv_builder_add(&b, "-q");
        argv_builder_add(&b, path);
    }
    return argv_builder_finish(&b);
}

static void alsa_destroy(IAudioBackend *self) { free(self); }

IAudioBackend *alsa_audio_backend_create(void) {
    IAudioBackend *b = calloc(1, sizeof(*b));
    if (!b) return NULL;
    b->name = alsa_name;
    b->available = alsa_available;
    b->capture_argv = alsa_capture;
    b->playback_argv = alsa_playback;
    b->destroy = alsa_destroy;
    return b;
}
