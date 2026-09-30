#ifndef APP_CORE_SCHEDULED_STATE_H
#define APP_CORE_SCHEDULED_STATE_H

/* Where a message to send later stands. */
typedef enum ScheduledState {
    SCHEDULED_WAITING = 0,
    SCHEDULED_SENT,
    SCHEDULED_FAILED,
    SCHEDULED_CANCELLED
} ScheduledState;

#endif
