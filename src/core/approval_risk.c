#include "core/approval_risk.h"

const char *approval_risk_label(ApprovalRisk r) { return r == APPROVAL_RISK_HIGH ? "HIGH" : r == APPROVAL_RISK_MEDIUM ? "MED" : "LOW"; }
const char *approval_risk_mark(ApprovalRisk r) { return r == APPROVAL_RISK_HIGH ? "!!" : r == APPROVAL_RISK_MEDIUM ? "!" : "\xC2\xB7"; }
