#ifndef APP_CORE_OUTGOING_MEDIA_H
#define APP_CORE_OUTGOING_MEDIA_H

/* A photo, video, audio file or document on its way out. The path is
 * inside the media folder. */
typedef struct OutgoingMedia {
    const char *path;
    const char *kind;         /* image, video, audio, document, sticker */
    const char *mime;
    const char *file_name;
    const char *caption;
    int         forwarded;
    int         forwarding_score;
} OutgoingMedia;

#endif
