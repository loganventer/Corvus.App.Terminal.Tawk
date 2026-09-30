#include "clients/control/control_request.h"

void control_request_dispose(ControlRequest *request) {
    cJSON_Delete(request->root);
    request->root = NULL;
    request->args = NULL;
}
