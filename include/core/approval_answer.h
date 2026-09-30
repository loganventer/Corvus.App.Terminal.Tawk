#ifndef APP_CORE_APPROVAL_ANSWER_H
#define APP_CORE_APPROVAL_ANSWER_H

/* Your answer to one approval request. */
typedef struct ApprovalAnswer {
    int   id;
    int   approved;
    char *text;          /* owned: the text as you edited it, or NULL when unchanged */
    int   remember;      /* allow the same again from this client, for this chat, until it disconnects */
} ApprovalAnswer;

void approval_answer_dispose(ApprovalAnswer *answer);

#endif
