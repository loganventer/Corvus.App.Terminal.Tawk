#ifndef APP_CONTRACTS_I_AUDIO_PLAYER_H
#define APP_CONTRACTS_I_AUDIO_PLAYER_H

#include <stdint.h>

/* Plays one audio file at a time in the background (voice notes). */
typedef struct IAudioPlayer {
    void *ctx;
    int         (*play)(struct IAudioPlayer *self, const char *path);
    void        (*stop)(struct IAudioPlayer *self);
    /* Non-zero while playing; also reaps a finished player. */
    int         (*is_playing)(struct IAudioPlayer *self);
    /* Path currently playing, or "" when idle. */
    const char *(*current)(struct IAudioPlayer *self);
    /* Milliseconds since the current file started, or 0 when idle. */
    int64_t     (*elapsed_ms)(struct IAudioPlayer *self);
    void        (*destroy)(struct IAudioPlayer *self);
} IAudioPlayer;

#endif
