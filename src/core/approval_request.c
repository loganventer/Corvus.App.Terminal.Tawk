#include "core/approval_request.h"
#include "utilities/str_util.h"

#include <stdlib.h>

void approval_request_copy(ApprovalRequest *dst, const ApprovalRequest *src) {
    *dst = *src;
    dst->text = src->text ? str_dup(src->text) : NULL;
}

void approval_request_dispose(ApprovalRequest *request) {
    free(request->text);
    request->text = NULL;
}
