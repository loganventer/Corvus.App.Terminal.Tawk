#ifndef APP_CLIENTS_CLI_CONTROL_COMMAND_KIND_H
#define APP_CLIENTS_CLI_CONTROL_COMMAND_KIND_H

/* The shell commands that talk to a running tawk. */
typedef enum ControlCommandKind {
    CONTROL_COMMAND_NONE = 0,
    CONTROL_COMMAND_SEND,
    CONTROL_COMMAND_TAIL,
    CONTROL_COMMAND_UNREAD,
    CONTROL_COMMAND_STATUS_LINE
} ControlCommandKind;

#endif
