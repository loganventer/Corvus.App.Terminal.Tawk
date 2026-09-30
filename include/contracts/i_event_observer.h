#ifndef APP_CONTRACTS_I_EVENT_OBSERVER_H
#define APP_CONTRACTS_I_EVENT_OBSERVER_H

#include "core/event.h"

/* Something besides the messaging manager that wants backend events (the
 * profile manager). Returns 1 when the event changed what is shown. */
typedef struct IEventObserver {
    void *ctx;
    int  (*on_event)(struct IEventObserver *self, const Event *event);
    void (*destroy)(struct IEventObserver *self);
} IEventObserver;

#endif
