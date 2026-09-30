#ifndef APP_CLIENTS_TUI_IMAGE_VIEWER_ACTION_H
#define APP_CLIENTS_TUI_IMAGE_VIEWER_ACTION_H

/* What the app does after a key or click in the image viewer. */
typedef enum ImageViewerAction {
    IMAGE_VIEWER_NONE = 0,
    IMAGE_VIEWER_CHANGED,         /* another photo is shown */
    IMAGE_VIEWER_CLOSED,
    IMAGE_VIEWER_OPEN_OUTSIDE     /* open the file in the system viewer */
} ImageViewerAction;

#endif
