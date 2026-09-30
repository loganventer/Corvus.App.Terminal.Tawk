#ifndef APP_CLIENTS_TUI_LOGIN_STEP_H
#define APP_CLIENTS_TUI_LOGIN_STEP_H

/* Pages of the linking wizard. */
typedef enum LoginStep {
    LOGIN_STEP_CONNECTING = 0,  /* checking for an existing login */
    LOGIN_STEP_WELCOME,         /* choose QR code or phone number */
    LOGIN_STEP_QR,
    LOGIN_STEP_PHONE_ENTRY,
    LOGIN_STEP_PHONE_CODE,
    LOGIN_STEP_LINKED           /* success, syncing chats */
} LoginStep;

#endif
