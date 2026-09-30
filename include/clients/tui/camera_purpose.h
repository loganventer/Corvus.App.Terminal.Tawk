#ifndef APP_CLIENTS_TUI_CAMERA_PURPOSE_H
#define APP_CLIENTS_TUI_CAMERA_PURPOSE_H

/* What a photo or video taken with the camera is for. */
typedef enum CameraPurpose {
    CAMERA_FOR_ATTACHMENT = 0,   /* the open chat */
    CAMERA_FOR_AVATAR,           /* your profile photo */
    CAMERA_FOR_STATUS            /* a status */
} CameraPurpose;

#endif
