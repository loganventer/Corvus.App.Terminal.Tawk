#ifndef APP_CLIENTS_CONTROL_CONTROL_OP_ENTRY_H
#define APP_CLIENTS_CONTROL_CONTROL_OP_ENTRY_H

#include "clients/control/control_op.h"

/* One operation of the protocol: its name, its handler, and whether it only reads. */
typedef struct ControlOpEntry {
    const char *name;
    ControlOp   run;
    int         read;
} ControlOpEntry;

#endif
