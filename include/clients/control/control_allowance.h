#ifndef APP_CLIENTS_CONTROL_CONTROL_ALLOWANCE_H
#define APP_CLIENTS_CONTROL_CONTROL_ALLOWANCE_H

/* "Allow this again for this session": one operation in one chat. */
typedef struct ControlAllowance {
    char op[32];
    char chat_jid[128];
} ControlAllowance;

#endif
