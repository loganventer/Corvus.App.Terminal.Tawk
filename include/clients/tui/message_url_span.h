#ifndef APP_CLIENTS_TUI_MESSAGE_URL_SPAN_H
#define APP_CLIENTS_TUI_MESSAGE_URL_SPAN_H

#define MESSAGE_URL_MAX 512

/* A web address shown on one screen row: the columns it takes, and the whole address (a long one
 * wraps over several rows, and each row's part opens all of it). */
typedef struct MessageUrlSpan {
    int  y;                    /* the screen row */
    int  x0;                   /* first column, and one past the last */
    int  x1;
    char url[MESSAGE_URL_MAX];
} MessageUrlSpan;

#endif
