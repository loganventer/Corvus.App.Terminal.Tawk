#ifndef APP_MANAGERS_COMPOSITE_EVENT_OBSERVER_H
#define APP_MANAGERS_COMPOSITE_EVENT_OBSERVER_H

#include "contracts/i_event_observer.h"

/* Hands each event to several observers (profiles, calls); it changed what
 * is shown when any of them says so. Does not own the observers. */
IEventObserver *composite_event_observer_create(void);
int             composite_event_observer_add(IEventObserver *composite, IEventObserver *observer);

#endif
