#include "core/automation_outcome.h"

#include <string.h>

static const char *const NAMES[AUTOMATION_OUTCOME_COUNT] = {
    "done", "approved", "declined", "timed out", "refused", "rate limited", "failed", "connected", "read", "allowed for the session",
    "approved by the agent"
};

const char *automation_outcome_name(AutomationOutcome o) {
    return (o >= 0 && o < AUTOMATION_OUTCOME_COUNT) ? NAMES[o] : "failed";
}

AutomationOutcome automation_outcome_parse(const char *name) {
    for (int i = 0; name && i < AUTOMATION_OUTCOME_COUNT; i++) {
        if (strcmp(name, NAMES[i]) == 0) return (AutomationOutcome)i;
    }
    return AUTOMATION_OUTCOME_FAILED;
}
