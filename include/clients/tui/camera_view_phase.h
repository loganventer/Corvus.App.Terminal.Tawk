#ifndef APP_CLIENTS_TUI_CAMERA_VIEW_PHASE_H
#define APP_CLIENTS_TUI_CAMERA_VIEW_PHASE_H

/* What the camera view shows. */
typedef enum CameraViewPhase {
    CAMERA_VIEW_LIVE = 0,        /* the live picture, to aim */
    CAMERA_VIEW_RECORDING,       /* the live picture while a video records */
    CAMERA_VIEW_REVIEW           /* the photo or video just taken, to use or retake */
} CameraViewPhase;

#endif
