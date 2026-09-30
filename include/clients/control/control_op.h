#ifndef APP_CLIENTS_CONTROL_CONTROL_OP_H
#define APP_CLIENTS_CONTROL_CONTROL_OP_H

#include "clients/control/control_request.h"
#include "clients/control/control_session.h"

struct ControlServer;

/* Handles one operation from a client and answers it. */
typedef void (*ControlOp)(struct ControlServer *server, ControlSession *session, const ControlRequest *req);

#endif
