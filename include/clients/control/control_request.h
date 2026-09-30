#ifndef APP_CLIENTS_CONTROL_CONTROL_REQUEST_H
#define APP_CLIENTS_CONTROL_CONTROL_REQUEST_H

#include "cJSON.h"

/* One request read from a client: its id, the operation and the arguments
 * (an object, never NULL once parsed). Owns the parsed JSON. */
typedef struct ControlRequest {
    char   id[64];
    char   op[32];
    cJSON *root;
    cJSON *args;
} ControlRequest;

void control_request_dispose(ControlRequest *request);

#endif
