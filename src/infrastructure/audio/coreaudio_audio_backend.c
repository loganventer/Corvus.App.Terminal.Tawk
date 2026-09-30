#include "infrastructure/audio/coreaudio_audio_backend.h"
#include "infrastructure/audio/ffplay_fallback.h"
#include "utilities/argv_builder.h"
#include "utilities/platform.h"
#include "utilities/process_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* macOS: capture with SoX's rec when it is installed, else through ffmpeg's
 * AVFoundation input; play with ffplay or afplay. ffmpeg's AVFoundation
 * capture clicks on some Macs, and rec through CoreAudio does not. */

static const char *ca_name(IAudioBackend *self) { (void)self; return "coreaudio"; }

static int ca_available(IAudioBackend *self) {
    (void)self;
    return platform_is_macos() && process_on_path("ffmpeg");
}

static int ca_capture(IAudioBackend *self, const char *device, char **argv, int max, char *st, size_t size) {
    (void)self;
    int chosen = device && *device && strcmp(device, "default") != 0;
    ArgvBuilder b;
    argv_builder_init(&b, argv, max, st, size);
    /* rec names devices differently from AVFoundation's numbers, so a chosen device stays with ffmpeg. */
    if (!chosen && process_on_path("rec")) {
        const char *rec[] = { "rec", "-q", "-r", "48000", "-c", "1", "-b", "16", "-e", "signed-integer",
                              "-t", "raw", "-", NULL };
        for (int i = 0; rec[i]; i++) argv_builder_add(&b, rec[i]);
        return argv_builder_finish(&b);
    }
    char input[160];
    snprintf(input, sizeof(input), ":%s", chosen ? device : "0");
    const char *args[] = { "ffmpeg", "-hide_banner", "-loglevel", "error", "-nostdin", "-f", "avfoundation",
                           "-i", input, "-f", "s16le", "-ar", "48000", "-ac", "1", "-", NULL };
    for (int i = 0; args[i]; i++) argv_builder_add(&b, args[i]);
    return argv_builder_finish(&b);
}

static int ca_playback(IAudioBackend *self, const char *path, char **argv, int max, char *st, size_t size) {
    (void)self;
    ArgvBuilder b;
    argv_builder_init(&b, argv, max, st, size);
    if (ffplay_fallback_add(&b, path) != 0) {
        argv_builder_add(&b, "afplay");
        argv_builder_add(&b, path);
    }
    return argv_builder_finish(&b);
}

static void ca_destroy(IAudioBackend *self) { free(self); }

IAudioBackend *coreaudio_audio_backend_create(void) {
    IAudioBackend *b = calloc(1, sizeof(*b));
    if (!b) return NULL;
    b->name = ca_name;
    b->available = ca_available;
    b->capture_argv = ca_capture;
    b->playback_argv = ca_playback;
    b->destroy = ca_destroy;
    return b;
}
