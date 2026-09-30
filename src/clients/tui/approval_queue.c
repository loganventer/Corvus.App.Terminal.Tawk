#include "clients/tui/approval_queue.h"
#include "utilities/str_util.h"

#include <stdlib.h>
#include <string.h>

struct ApprovalQueue {
    IApprovalPrompt prompt;
    ApprovalRequest requests[APPROVAL_QUEUE_MAX];
    int             count;
    ApprovalAnswer  answers[APPROVAL_QUEUE_MAX];
    int             answer_count;
    uint64_t        arrivals;
};

static int index_of(const ApprovalQueue *q, int id) {
    for (int i = 0; i < q->count; i++) if (q->requests[i].id == id) return i;
    return -1;
}

static void remove_at(ApprovalQueue *q, int i) {
    approval_request_dispose(&q->requests[i]);
    memmove(&q->requests[i], &q->requests[i + 1], (size_t)(q->count - i - 1) * sizeof(q->requests[0]));
    q->count--;
}

static void ask(IApprovalPrompt *self, const ApprovalRequest *request) {
    ApprovalQueue *q = self->ctx;
    if (q->count >= APPROVAL_QUEUE_MAX) {                  /* no room: declined at once */
        if (q->answer_count < APPROVAL_QUEUE_MAX) q->answers[q->answer_count++] = (ApprovalAnswer){ request->id, 0, NULL, 0 };
        return;
    }
    approval_request_copy(&q->requests[q->count++], request);
    q->arrivals++;
}

static void withdraw(IApprovalPrompt *self, int id) {
    ApprovalQueue *q = self->ctx;
    int i = index_of(q, id);
    if (i >= 0) remove_at(q, i);
}

static int take_answer(IApprovalPrompt *self, ApprovalAnswer *out) {
    ApprovalQueue *q = self->ctx;
    if (q->answer_count == 0) return 0;
    *out = q->answers[0];
    memmove(q->answers, q->answers + 1, (size_t)--q->answer_count * sizeof(q->answers[0]));
    return 1;
}

static void prompt_destroy(IApprovalPrompt *self) { (void)self; }   /* owned by the queue */

ApprovalQueue *approval_queue_create(void) {
    ApprovalQueue *q = calloc(1, sizeof(*q));
    if (!q) return NULL;
    q->prompt = (IApprovalPrompt){ q, ask, withdraw, take_answer, prompt_destroy };
    return q;
}

void approval_queue_destroy(ApprovalQueue *q) {
    if (!q) return;
    while (q->count > 0) remove_at(q, q->count - 1);
    for (int i = 0; i < q->answer_count; i++) approval_answer_dispose(&q->answers[i]);
    free(q);
}

IApprovalPrompt *approval_queue_prompt(ApprovalQueue *q) { return &q->prompt; }
int approval_queue_count(const ApprovalQueue *q) { return q ? q->count : 0; }
uint64_t approval_queue_arrivals(const ApprovalQueue *q) { return q ? q->arrivals : 0; }

const ApprovalRequest *approval_queue_at(const ApprovalQueue *q, int i) {
    return q && i >= 0 && i < q->count ? &q->requests[i] : NULL;
}

const ApprovalRequest *approval_queue_find(const ApprovalQueue *q, int id) {
    int i = q ? index_of(q, id) : -1;
    return i >= 0 ? &q->requests[i] : NULL;
}

int approval_queue_high_count(const ApprovalQueue *q) {
    int n = 0;
    for (int i = 0; q && i < q->count; i++) n += q->requests[i].risk == APPROVAL_RISK_HIGH;
    return n;
}

void approval_queue_answer(ApprovalQueue *q, int id, int approved, const char *text, int remember) {
    int i = index_of(q, id);
    if (i < 0 || q->answer_count >= APPROVAL_QUEUE_MAX) return;
    int high = q->requests[i].risk == APPROVAL_RISK_HIGH;
    q->answers[q->answer_count++] = (ApprovalAnswer){ id, approved, approved && text ? str_dup(text) : NULL, approved && remember && !high };
    remove_at(q, i);
}
