#ifndef APP_UTILITIES_EVENT_QUEUE_H
#define APP_UTILITIES_EVENT_QUEUE_H

#include "core/event.h"

typedef struct EventQueue EventQueue;

EventQueue *event_queue_create(int capacity);
/* Moves *evt into the queue, blocking while full (back-pressure for large
 * history syncs). Returns -1 once closed; the caller then keeps ownership. */
int         event_queue_push(EventQueue *queue, Event *evt);
/* Non-blocking. Returns 0 and moves an event into *out, or -1 when empty. */
int         event_queue_pop(EventQueue *queue, Event *out);
/* Wakes blocked producers; further pushes fail. */
void        event_queue_close(EventQueue *queue);
void        event_queue_destroy(EventQueue *queue);

#endif
