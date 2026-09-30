#ifndef APP_CONTRACTS_I_VIDEO_POSTER_H
#define APP_CONTRACTS_I_VIDEO_POSTER_H

#include <stddef.h>

/* Still frames of downloaded videos, used as their preview picture. */
typedef struct IVideoPoster {
    void *ctx;
    /* Writes the poster's path to `out` and returns 1 when it is ready.
     * Otherwise starts making it (once per video) and returns 0. */
    int  (*get)(struct IVideoPoster *self, const char *video_path, char *out, size_t size);
    /* Non-zero while a poster is being made (the view should redraw soon). */
    int  (*pending)(struct IVideoPoster *self);
    void (*destroy)(struct IVideoPoster *self);
} IVideoPoster;

#endif
