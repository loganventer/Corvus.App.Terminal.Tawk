#ifndef APP_CLIENTS_TUI_CAMERA_VIEW_ACTION_H
#define APP_CLIENTS_TUI_CAMERA_VIEW_ACTION_H

/* What a key in the camera view asks for. */
typedef enum CameraViewAction {
    CAMERA_VIEW_NONE = 0,
    CAMERA_VIEW_SNAP,            /* take the photo */
    CAMERA_VIEW_START_VIDEO,     /* start recording a video */
    CAMERA_VIEW_STOP_VIDEO,      /* finish the video */
    CAMERA_VIEW_PLAY,            /* play the video just recorded */
    CAMERA_VIEW_USE,             /* attach the photo or video taken */
    CAMERA_VIEW_RETAKE,          /* discard it and go back to the live picture */
    CAMERA_VIEW_CANCEL           /* close without a photo */
} CameraViewAction;

#endif
