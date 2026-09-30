#ifndef APP_CORE_STATUS_KIND_H
#define APP_CORE_STATUS_KIND_H

/* What a status you post is made of. */
typedef enum StatusKind {
    STATUS_KIND_TEXT = 0,   /* words on a coloured background */
    STATUS_KIND_PHOTO,
    STATUS_KIND_VIDEO,
    STATUS_KIND_LINK,       /* text with a web address in it */
    STATUS_KIND_COUNT
} StatusKind;

/* The protocol name: text, image, video or link. */
const char *status_kind_name(StatusKind kind);
/* The label on the status composer's tab: Text, Photo, Video or Link. */
const char *status_kind_label(StatusKind kind);
int         status_kind_has_media(StatusKind kind);

#endif
