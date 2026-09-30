#include "infrastructure/audio/pipewire_audio_backend.h"
#include "utilities/argv_builder.h"
#include "utilities/process_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Native PipeWire via pw-record / pw-play. */

static const char *pw_name(IAudioBackend *self) { (void)self; return "pipewire"; }

static int pw_available(IAudioBackend *self) {
    (void)self;
    if (!process_on_path("pw-record") || !process_on_path("pw-play")) return 0;
    char sock[256];
    snprintf(sock, sizeof(sock), "/run/user/%u/pipewire-0", (unsigned)getuid());
    return access(sock, F_OK) == 0;
}

static int pw_capture(IAudioBackend *self, const char *device, char **argv, int max, char *st, size_t size) {
    (void)self;
    ArgvBuilder b;
    argv_builder_init(&b, argv, max, st, size);
    argv_builder_add(&b, "pw-record");
    argv_builder_add(&b, "--format=s16");
    argv_builder_add(&b, "--rate=48000");
    argv_builder_add(&b, "--channels=1");
    if (device && *device && strcmp(device, "default") != 0) {
        argv_builder_add(&b, "--target");
        argv_builder_add(&b, device);
    }
    argv_builder_add(&b, "-");
    return argv_builder_finish(&b);
}

static int pw_playback(IAudioBackend *self, const char *path, char **argv, int max, char *st, size_t size) {
    (void)self;
    ArgvBuilder b;
    argv_builder_init(&b, argv, max, st, size);
    argv_builder_add(&b, "pw-play");
    argv_builder_add(&b, path);
    return argv_builder_finish(&b);
}

static void pw_destroy(IAudioBackend *self) { free(self); }

IAudioBackend *pipewire_audio_backend_create(void) {
    IAudioBackend *b = calloc(1, sizeof(*b));
    if (!b) return NULL;
    b->name = pw_name;
    b->available = pw_available;
    b->capture_argv = pw_capture;
    b->playback_argv = pw_playback;
    b->destroy = pw_destroy;
    return b;
}
