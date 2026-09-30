#include "clients/control/control_pending.h"
#include "utilities/str_util.h"

#include <stdlib.h>
#include <string.h>

void control_pending_init(ControlPending *p, const char *request_id, const char *op, WriteKind kind, ControlExecute execute) {
    memset(p, 0, sizeof(*p));
    str_copy(p->request_id, sizeof(p->request_id), request_id);
    str_copy(p->op, sizeof(p->op), op);
    p->kind = kind;
    p->execute = execute;
    p->args = cJSON_CreateObject();
}

void control_pending_dispose(ControlPending *p) {
    free(p->text);
    p->text = NULL;
    cJSON_Delete(p->args);
    p->args = NULL;
}
