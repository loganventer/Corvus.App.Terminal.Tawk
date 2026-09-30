#ifndef APP_CLIENTS_CONTROL_CONTROL_CONFIRMATION_H
#define APP_CLIENTS_CONTROL_CONTROL_CONFIRMATION_H

#include <stdint.h>

#include "clients/control/control_pending.h"

/* A destructive request held until its client confirms it with the token. */
typedef struct ControlConfirmation {
    char           token[40];
    ControlPending pending;
    int64_t        expires_ms;
} ControlConfirmation;

#endif
