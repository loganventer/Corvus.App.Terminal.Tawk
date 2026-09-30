#ifndef APP_CORE_STATUS_VIEWER_H
#define APP_CORE_STATUS_VIEWER_H

#include <stdint.h>

/* Someone who saw one of your statuses, and their like when they left one. */
typedef struct StatusViewer {
    char    jid[128];
    int64_t viewed_at;      /* epoch seconds; 0 when only the like arrived */
    char    reaction[32];   /* "❤️" and the like, or "" */
} StatusViewer;

#endif
