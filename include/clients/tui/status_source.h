#ifndef APP_CLIENTS_TUI_STATUS_SOURCE_H
#define APP_CLIENTS_TUI_STATUS_SOURCE_H

#include "core/status_update.h"

/* Where the conversation finds the status a reply answers, without knowing
 * who keeps statuses. `find` fills `out` (the caller disposes it) and
 * returns -1 when the status is not known (expired, or never seen). */
typedef struct StatusSource {
    void *ctx;
    int  (*find)(void *ctx, const char *status_id, StatusUpdate *out);
} StatusSource;

#endif
