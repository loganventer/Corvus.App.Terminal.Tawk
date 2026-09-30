#ifndef APP_CLIENTS_CONTROL_CONTROL_WATCH_H
#define APP_CLIENTS_CONTROL_CONTROL_WATCH_H

/* A subscribed chat's unread count as last told to the client. */
typedef struct ControlWatch {
    char jid[128];
    int  unread;
} ControlWatch;

#endif
