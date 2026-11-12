#ifndef APP_CLIENTS_TUI_PRIVATE_ACTION_H
#define APP_CLIENTS_TUI_PRIVATE_ACTION_H

/* What is done once a private chat has been asked for. */
typedef enum {
    PRIVATE_ACTION_OPEN = 0,          /* open the chat */
    PRIVATE_ACTION_MAKE_PRIVATE,      /* make a chat private (needs a lock password first) */
    PRIVATE_ACTION_MAKE_PUBLIC,       /* make it an ordinary chat again */
    PRIVATE_ACTION_EXPORT,            /* write it to a file */
    PRIVATE_ACTION_CHANGE_PASSWORD    /* a new lock password has been set */
} PrivateAction;

#endif
