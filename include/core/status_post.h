#ifndef APP_CORE_STATUS_POST_H
#define APP_CORE_STATUS_POST_H

#include <stdint.h>

#include "core/status_kind.h"

#define STATUS_POST_MAX_CHARS 700

/* One status to post. Text is the words of a text or link status and the
 * caption of a photo or video. */
typedef struct StatusPost {
    StatusKind kind;
    char       text[STATUS_POST_MAX_CHARS * 4 + 1];   /* UTF-8 */
    char       path[1024];                            /* PHOTO, VIDEO: the file */
    char       mime[64];                              /* PHOTO, VIDEO */
    uint32_t   background_argb;                       /* TEXT, LINK */
    int        font;                                  /* TEXT, LINK: WhatsApp's font number */
    char       id[64];                                /* set by the status manager */
} StatusPost;

#endif
