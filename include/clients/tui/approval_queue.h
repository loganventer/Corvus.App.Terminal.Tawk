#ifndef APP_CLIENTS_TUI_APPROVAL_QUEUE_H
#define APP_CLIENTS_TUI_APPROVAL_QUEUE_H

#include <stdint.h>

#include "contracts/i_approval_prompt.h"

#define APPROVAL_QUEUE_MAX 32

/* Requests from agents waiting for you, oldest first, and your answers
 * waiting to be collected. It is how the terminal client asks you
 * (IApprovalPrompt); the Agents tab shows it and answers from it. */
typedef struct ApprovalQueue ApprovalQueue;

ApprovalQueue         *approval_queue_create(void);
void                   approval_queue_destroy(ApprovalQueue *queue);
IApprovalPrompt       *approval_queue_prompt(ApprovalQueue *queue);

int                    approval_queue_count(const ApprovalQueue *queue);
const ApprovalRequest *approval_queue_at(const ApprovalQueue *queue, int index);
const ApprovalRequest *approval_queue_find(const ApprovalQueue *queue, int id);
int                    approval_queue_high_count(const ApprovalQueue *queue);
/* Goes up by one for every request that arrives. */
uint64_t               approval_queue_arrivals(const ApprovalQueue *queue);
/* Answers and removes one. `text` is the edited text, or NULL. */
void                   approval_queue_answer(ApprovalQueue *queue, int id, int approved, const char *text, int remember);

#endif
