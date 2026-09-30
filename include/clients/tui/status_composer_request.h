#ifndef APP_CLIENTS_TUI_STATUS_COMPOSER_REQUEST_H
#define APP_CLIENTS_TUI_STATUS_COMPOSER_REQUEST_H

/* What the status composer needs its owner to do after a key or click. */
typedef enum StatusComposerRequest {
    STATUS_REQUEST_NONE = 0,
    STATUS_REQUEST_REDRAW,
    STATUS_REQUEST_CLOSED,
    STATUS_REQUEST_POST,           /* post what the composer holds */
    STATUS_REQUEST_CHOOSE_FILE,    /* pick a photo or video, then call _set_file */
    STATUS_REQUEST_CAMERA          /* take a photo or record a video with the camera, then call _set_file */
} StatusComposerRequest;

#endif
