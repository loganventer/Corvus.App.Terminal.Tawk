#ifndef APP_ENGINES_IDLE_TRACKER_H
#define APP_ENGINES_IDLE_TRACKER_H

#include <stdint.h>

typedef struct IdleTracker {
    int64_t last_activity_ms;
} IdleTracker;

void idle_tracker_reset(IdleTracker *tracker);
/* True once `minutes` of inactivity have passed. Zero minutes disables it. */
int  idle_tracker_is_idle(const IdleTracker *tracker, int minutes);

#endif
