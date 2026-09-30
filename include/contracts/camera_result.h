#ifndef APP_CONTRACTS_CAMERA_RESULT_H
#define APP_CONTRACTS_CAMERA_RESULT_H

/* How taking a photo went. */
typedef enum CameraResult {
    CAMERA_SAVED = 0,
    CAMERA_NOT_FOUND,       /* no camera, or no tool to use it, on this system */
    CAMERA_FAILED           /* the camera did not deliver a picture */
} CameraResult;

#endif
