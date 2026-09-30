#include "core/approval_answer.h"

#include <stdlib.h>

void approval_answer_dispose(ApprovalAnswer *answer) {
    free(answer->text);
    answer->text = NULL;
}
