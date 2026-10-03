#ifndef APP_CORE_AUTOMATION_OUTCOME_H
#define APP_CORE_AUTOMATION_OUTCOME_H

/* What became of one thing a control socket client asked for. */
typedef enum AutomationOutcome {
    AUTOMATION_OUTCOME_DONE = 0,        /* done without asking */
    AUTOMATION_OUTCOME_APPROVED,        /* you said yes */
    AUTOMATION_OUTCOME_DECLINED,        /* you said no */
    AUTOMATION_OUTCOME_TIMED_OUT,       /* nobody answered */
    AUTOMATION_OUTCOME_REFUSED,         /* the settings do not allow it */
    AUTOMATION_OUTCOME_RATE_LIMITED,
    AUTOMATION_OUTCOME_FAILED,
    AUTOMATION_OUTCOME_CONNECTED,       /* a client said hello */
    AUTOMATION_OUTCOME_READ,            /* it looked at something */
    AUTOMATION_OUTCOME_ALLOWED,         /* allowed by an earlier "for this session" */
    AUTOMATION_OUTCOME_SELF_APPROVED,   /* the client answered its own request, as access admin lets it */
    AUTOMATION_OUTCOME_COUNT
} AutomationOutcome;

const char       *automation_outcome_name(AutomationOutcome outcome);
AutomationOutcome automation_outcome_parse(const char *name);

#endif
