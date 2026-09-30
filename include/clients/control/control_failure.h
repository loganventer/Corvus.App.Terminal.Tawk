#ifndef APP_CLIENTS_CONTROL_CONTROL_FAILURE_H
#define APP_CLIENTS_CONTROL_CONTROL_FAILURE_H

/* Why carrying out a request failed: an error code from CONTROL.md and words for people. */
typedef struct ControlFailure {
    const char *code;
    char        why[200];
} ControlFailure;

#endif
