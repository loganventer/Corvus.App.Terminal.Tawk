#ifndef APP_CONTRACTS_I_AUDIO_BACKEND_H
#define APP_CONTRACTS_I_AUDIO_BACKEND_H

#include <stddef.h>

/* One audio server or OS audio stack. Describes how to capture raw PCM and
 * how to play a file; it never runs anything itself. */
typedef struct IAudioBackend {
    void *ctx;
    const char *(*name)(struct IAudioBackend *self);
    /* Non-zero when the tools and server for this backend are present. */
    int  (*available)(struct IAudioBackend *self);
    /* argv that writes signed 16-bit little-endian, 48 kHz, mono PCM to stdout. */
    int  (*capture_argv)(struct IAudioBackend *self, const char *device, char **argv, int max_args, char *storage, size_t storage_size);
    /* argv that plays `path` and exits when done. */
    int  (*playback_argv)(struct IAudioBackend *self, const char *path, char **argv, int max_args, char *storage, size_t storage_size);
    void (*destroy)(struct IAudioBackend *self);
} IAudioBackend;

#endif
