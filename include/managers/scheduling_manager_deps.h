#ifndef APP_MANAGERS_SCHEDULING_MANAGER_DEPS_H
#define APP_MANAGERS_SCHEDULING_MANAGER_DEPS_H

#include "contracts/i_scheduled_message_store.h"

/* What the scheduling manager depends on, injected by the composition root. */
typedef struct SchedulingManagerDeps {
    IScheduledMessageStore *store;
} SchedulingManagerDeps;

#endif
