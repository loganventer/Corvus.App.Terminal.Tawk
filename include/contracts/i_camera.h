#ifndef APP_CONTRACTS_I_CAMERA_H
#define APP_CONTRACTS_I_CAMERA_H

#include "contracts/camera_result.h"
#include "utilities/rgb_image.h"

/* The computer's camera as a live picture that can be snapped or recorded. */
typedef struct ICamera {
    void *ctx;
    /* True when this system has a camera tawk can use. */
    int          (*available)(struct ICamera *self);
    /* Starts streaming frames of width x height pixels; returns 0 when started. */
    int          (*start_preview)(struct ICamera *self, int width, int height);
    /* Returns 1 when a new frame arrived since the last call and points `frame`
     * at it; the pixels stay valid until the next call or stop_preview. */
    int          (*latest_frame)(struct ICamera *self, RgbImage *frame);
    /* Saves the newest frame as a JPEG at `path`. */
    CameraResult (*snap)(struct ICamera *self, const char *path);
    void         (*stop_preview)(struct ICamera *self);
    /* Starts recording an MP4 at `path` while the preview carries on, with
     * sound from audio_argv (raw 48 kHz mono s16le on its stdout; NULL for
     * none); returns 0 when recording. */
    int          (*start_recording)(struct ICamera *self, const char *path, char *const audio_argv[]);
    /* Finishes the MP4 and stops the camera; returns 0 when it holds a video. */
    int          (*stop_recording)(struct ICamera *self);
    int          (*is_recording)(struct ICamera *self);
    void         (*destroy)(struct ICamera *self);
} ICamera;

#endif
