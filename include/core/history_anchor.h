#ifndef APP_CORE_HISTORY_ANCHOR_H
#define APP_CORE_HISTORY_ANCHOR_H

#include <stdint.h>

/* The oldest message we have in a chat; older history is requested from
 * the phone relative to it. */
typedef struct HistoryAnchor {
    char    chat[128];
    char    id[64];
    int64_t timestamp;
    int     from_me;
} HistoryAnchor;

#endif
