#include "infrastructure/audio/pulse_audio_backend.h"
#include "infrastructure/audio/ffplay_fallback.h"
#include "utilities/argv_builder.h"
#include "utilities/process_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* PulseAudio, including WSLg and pipewire-pulse compatibility servers. */

static const char *pulse_name(IAudioBackend *self) { (void)self; return "pulse"; }

static int pulse_available(IAudioBackend *self) {
    (void)self;
    if (!process_on_path("parecord")) return 0;
    if (getenv("PULSE_SERVER")) return 1;
    char sock[256];
    snprintf(sock, sizeof(sock), "/run/user/%u/pulse/native", (unsigned)getuid());
    return access(sock, F_OK) == 0 || access("/mnt/wslg/PulseServer", F_OK) == 0;
}

static int pulse_capture(IAudioBackend *self, const char *device, char **argv, int max, char *st, size_t size) {
    (void)self;
    ArgvBuilder b;
    argv_builder_init(&b, argv, max, st, size);
    argv_builder_add(&b, "parecord");
    argv_builder_add(&b, "--raw");
    argv_builder_add(&b, "--format=s16le");
    argv_builder_add(&b, "--rate=48000");
    argv_builder_add(&b, "--channels=1");
    if (device && *device && strcmp(device, "default") != 0) {
        char arg[160];
        snprintf(arg, sizeof(arg), "--device=%s", device);
        argv_builder_add(&b, arg);
    }
    return argv_builder_finish(&b);
}

static int pulse_playback(IAudioBackend *self, const char *path, char **argv, int max, char *st, size_t size) {
    (void)self;
    ArgvBuilder b;
    argv_builder_init(&b, argv, max, st, size);
    if (ffplay_fallback_add(&b, path) != 0) {
        argv_builder_add(&b, "paplay");
        argv_builder_add(&b, path);
    }
    return argv_builder_finish(&b);
}

static void pulse_destroy(IAudioBackend *self) { free(self); }

IAudioBackend *pulse_audio_backend_create(void) {
    IAudioBackend *b = calloc(1, sizeof(*b));
    if (!b) return NULL;
    b->name = pulse_name;
    b->available = pulse_available;
    b->capture_argv = pulse_capture;
    b->playback_argv = pulse_playback;
    b->destroy = pulse_destroy;
    return b;
}
