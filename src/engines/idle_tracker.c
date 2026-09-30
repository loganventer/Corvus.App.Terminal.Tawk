#include "engines/idle_tracker.h"
#include "utilities/clock_util.h"

void idle_tracker_reset(IdleTracker *tracker) {
    tracker->last_activity_ms = clock_now_ms();
}

int idle_tracker_is_idle(const IdleTracker *tracker, int minutes) {
    if (minutes <= 0) return 0;
    return clock_now_ms() - tracker->last_activity_ms >= (int64_t)minutes * 60000;
}
