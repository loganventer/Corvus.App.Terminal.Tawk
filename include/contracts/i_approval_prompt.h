#ifndef APP_CONTRACTS_I_APPROVAL_PROMPT_H
#define APP_CONTRACTS_I_APPROVAL_PROMPT_H

#include "core/approval_answer.h"
#include "core/approval_request.h"

/* Asks you, in whatever way the client in front of you can, whether a
 * control socket client may go ahead. Answers are collected by polling so
 * the asker and the screen stay independent. */
typedef struct IApprovalPrompt {
    void *ctx;
    /* Keeps a copy of the request and shows it when its turn comes. */
    void (*ask)(struct IApprovalPrompt *self, const ApprovalRequest *request);
    /* Takes a request back (it timed out, or its client went away). */
    void (*withdraw)(struct IApprovalPrompt *self, int id);
    /* Hands over one answer; returns 1 when there was one. */
    int  (*take_answer)(struct IApprovalPrompt *self, ApprovalAnswer *out);
    void (*destroy)(struct IApprovalPrompt *self);
} IApprovalPrompt;

#endif
