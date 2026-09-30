#ifndef APP_CORE_STATUS_AUTHOR_H
#define APP_CORE_STATUS_AUTHOR_H

#include <stdint.h>

/* Someone with statuses from the last day, for the list of updates. */
typedef struct StatusAuthor {
    char    jid[128];
    char    name[128];      /* their push name from the newest status, "" when unknown */
    int     count;
    int     unviewed;
    int64_t latest;         /* epoch seconds of the newest */
    int     from_me;        /* your own statuses */
} StatusAuthor;

#endif
