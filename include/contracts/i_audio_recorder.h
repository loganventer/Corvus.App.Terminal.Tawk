#ifndef APP_CONTRACTS_I_AUDIO_RECORDER_H
#define APP_CONTRACTS_I_AUDIO_RECORDER_H

#include <stdint.h>

/* Records the microphone to an Ogg/Opus file (voice notes). */
typedef struct IAudioRecorder {
    void *ctx;
    int     (*start)(struct IAudioRecorder *self, const char *path, const char *device);
    /* Finishes the file. Returns 0 when a usable recording exists. */
    int     (*stop)(struct IAudioRecorder *self);
    /* Stops and deletes the file. */
    void    (*cancel)(struct IAudioRecorder *self);
    int     (*is_recording)(struct IAudioRecorder *self);
    int64_t (*elapsed_ms)(struct IAudioRecorder *self);
    void    (*destroy)(struct IAudioRecorder *self);
} IAudioRecorder;

#endif
