#ifndef APP_CLIENTS_TUI_MEDIA_PICTURE_SOURCE_H
#define APP_CLIENTS_TUI_MEDIA_PICTURE_SOURCE_H

/* Where a message's picture comes from, sharpest first. */
typedef enum MediaPictureSource {
    MEDIA_PICTURE_NONE = 0,
    MEDIA_PICTURE_FILE,          /* the downloaded photo */
    MEDIA_PICTURE_POSTER,        /* a frame taken from the downloaded video */
    MEDIA_PICTURE_PAGE,          /* a page of the downloaded PDF */
    MEDIA_PICTURE_THUMB,         /* the small preview WhatsApp sends */
    MEDIA_PICTURE_PLACEHOLDER    /* a video or PDF with no preview at all */
} MediaPictureSource;

#endif
