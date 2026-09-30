#ifndef APP_CONTRACTS_I_STATUS_PUBLISHER_H
#define APP_CONTRACTS_I_STATUS_PUBLISHER_H

#include "core/status_post.h"

/* Posts statuses (status@broadcast) to the people your phone's status
 * privacy allows. Sends only the request; the outcome arrives as an
 * EVENT_STATUS_POSTED. Owned by the gateway that hands it out. Backends
 * that cannot post statuses hand out none. */
typedef struct IStatusPublisher {
    void *ctx;
    /* Media paths must be inside the media folder. */
    int  (*post)(struct IStatusPublisher *self, const StatusPost *post);
} IStatusPublisher;

#endif
