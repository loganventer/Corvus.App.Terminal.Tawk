#ifndef APP_MANAGERS_PENDING_ACTION_H
#define APP_MANAGERS_PENDING_ACTION_H

/* The next thing the connection supervisor will do when its timer fires. */
typedef enum PendingAction {
    PENDING_NONE = 0,
    PENDING_CONNECT,   /* ask the backend to open the WhatsApp connection */
    PENDING_RECONNECT, /* drop the current connection, even one that looks open, and connect */
    PENDING_RESTART    /* restart the backend runtime, then connect */
} PendingAction;

#endif
