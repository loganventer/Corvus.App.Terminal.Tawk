#ifndef APP_CLIENTS_CONTROL_CONTROL_EXECUTE_H
#define APP_CLIENTS_CONTROL_CONTROL_EXECUTE_H

#include "cJSON.h"
#include "clients/control/control_failure.h"

struct ControlServer;
struct ControlPending;

/* Carries a checked request out; returns its result, or NULL with `failure` filled. */
typedef cJSON *(*ControlExecute)(struct ControlServer *server, const struct ControlPending *pending, ControlFailure *failure);

#endif
