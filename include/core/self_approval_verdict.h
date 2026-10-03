#ifndef APP_CORE_SELF_APPROVAL_VERDICT_H
#define APP_CORE_SELF_APPROVAL_VERDICT_H

/* Whether a control socket client may answer its own waiting request. */
typedef enum SelfApprovalVerdict {
    SELF_APPROVAL_ALLOW = 0,
    SELF_APPROVAL_OFF,                 /* access is not admin */
    SELF_APPROVAL_BAD_TOKEN,           /* it did not show the admin token */
    SELF_APPROVAL_NOT_THIS_KIND,       /* only sends and small things; the rest stays yours */
    SELF_APPROVAL_CHAT_NOT_LISTED,     /* the chat is not one you chose for this */
    SELF_APPROVAL_RATE_LIMITED         /* this hour's allowance is used up */
} SelfApprovalVerdict;

#endif
