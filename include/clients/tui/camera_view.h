#ifndef APP_CLIENTS_TUI_CAMERA_VIEW_H
#define APP_CLIENTS_TUI_CAMERA_VIEW_H

#include "clients/tui/camera_view_action.h"
#include "clients/tui/camera_view_phase.h"
#include "clients/tui/ui_rect.h"
#include "utilities/rgb_image.h"

/* Taking a photo or a video: the live camera picture, then what was taken to review.
 * It shows frames it is given and says what the keys ask for; the camera
 * itself is driven by the app. */
typedef struct CameraView {
    int             open;
    CameraViewPhase phase;
    RgbImage        frame;           /* the picture shown, owned */
    char            photo[1024];     /* the photo or video taken, while reviewing */
    int             video;           /* what is reviewed is a video */
    int             seconds;         /* its length, or the time recorded so far */
    UiRect          picture;         /* the cells it fills, for Sixel */
} CameraView;

void             camera_view_open(CameraView *view);
void             camera_view_close(CameraView *view);
/* Shows a new live frame (copied). */
void             camera_view_set_frame(CameraView *view, const RgbImage *frame);
/* Freezes on the frame shown, now saved as `photo`. */
void             camera_view_review(CameraView *view, const char *photo);
/* Recording a video: the picture stays live, with the time recorded shown. */
void             camera_view_recording(CameraView *view);
/* Freezes on the last frame of a video saved as `path`, `seconds` long. */
void             camera_view_review_video(CameraView *view, const char *path, int seconds);
/* Back to the live picture after a retake. */
void             camera_view_live(CameraView *view);
CameraViewAction camera_view_key(const CameraView *view, int is_key_code, int ch);
/* Draws the frame in cells, or leaves them blank when pixel_images is set. */
void             camera_view_render(CameraView *view, UiRect area, int pixel_images);
/* Writes the frame as Sixel over the blank cells; call after the screen update. */
void             camera_view_present_pixels(const CameraView *view, int cell_w, int cell_h);

#endif
