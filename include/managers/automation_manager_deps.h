#ifndef APP_MANAGERS_AUTOMATION_MANAGER_DEPS_H
#define APP_MANAGERS_AUTOMATION_MANAGER_DEPS_H

#include "contracts/i_automation_log.h"
#include "core/settings.h"

/* What the automation manager depends on, injected by the composition root. */
typedef struct AutomationManagerDeps {
    IAutomationLog *log;
    const Settings *settings;
} AutomationManagerDeps;

#endif
