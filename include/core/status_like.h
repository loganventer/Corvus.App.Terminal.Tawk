#ifndef APP_CORE_STATUS_LIKE_H
#define APP_CORE_STATUS_LIKE_H

/* Someone liked one of your statuses. */
typedef struct StatusLike {
    char who[128];      /* their JID */
    char emoji[32];
} StatusLike;

#endif
