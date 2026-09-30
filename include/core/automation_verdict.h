#ifndef APP_CORE_AUTOMATION_VERDICT_H
#define APP_CORE_AUTOMATION_VERDICT_H

/* Whether a write asked for over the control socket may go ahead. */
typedef enum AutomationVerdict {
    AUTOMATION_VERDICT_ALLOW = 0,
    AUTOMATION_VERDICT_ASK,            /* only once you say yes in tawk */
    AUTOMATION_VERDICT_REFUSE,         /* writing is turned off */
    AUTOMATION_VERDICT_RATE_LIMITED
} AutomationVerdict;

#endif
