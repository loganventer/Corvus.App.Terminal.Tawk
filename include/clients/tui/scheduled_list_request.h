#ifndef APP_CLIENTS_TUI_SCHEDULED_LIST_REQUEST_H
#define APP_CLIENTS_TUI_SCHEDULED_LIST_REQUEST_H

/* What the scheduled messages list needs its owner to do after a key or click. */
typedef enum ScheduledListRequest {
    SCHEDULED_REQUEST_NONE = 0,
    SCHEDULED_REQUEST_REDRAW,
    SCHEDULED_REQUEST_CLOSED,
    SCHEDULED_REQUEST_SEND_NOW,     /* send scheduled_list_dialog_selected now */
    SCHEDULED_REQUEST_RESCHEDULE,   /* move it to scheduled_list_dialog_when */
    SCHEDULED_REQUEST_CANCEL        /* drop it */
} ScheduledListRequest;

#endif
