#ifndef APP_MANAGERS_STATUS_MANAGER_DEPS_H
#define APP_MANAGERS_STATUS_MANAGER_DEPS_H

#include "contracts/i_status_publisher.h"

/* What the status manager depends on, injected by the composition root. */
typedef struct StatusManagerDeps {
    IStatusPublisher *publisher;   /* NULL when the backend cannot post statuses */
    const char       *media_dir;
} StatusManagerDeps;

#endif
