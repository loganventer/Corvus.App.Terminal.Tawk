#ifndef APP_CORE_APPROVAL_RISK_H
#define APP_CORE_APPROVAL_RISK_H

/* How much harm a request could do, shown in words, a mark and a colour. */
typedef enum ApprovalRisk {
    APPROVAL_RISK_LOW = 0,      /* a reaction, a read mark */
    APPROVAL_RISK_MEDIUM,       /* sends, schedules, changes a setting */
    APPROVAL_RISK_HIGH          /* deletes or blocks */
} ApprovalRisk;

/* "LOW", "MED", "HIGH". */
const char *approval_risk_label(ApprovalRisk risk);
/* "·", "!", "!!": the same meaning without colour. */
const char *approval_risk_mark(ApprovalRisk risk);

#endif
