#ifndef APP_MANAGERS_AUTOMATION_MANAGER_DEPS_H
#define APP_MANAGERS_AUTOMATION_MANAGER_DEPS_H

#include "contracts/i_admin_token_store.h"
#include "contracts/i_automation_log.h"
#include "core/settings.h"

/* What the automation manager depends on, injected by the composition root. */
typedef struct AutomationManagerDeps {
    IAutomationLog *log;
    const Settings *settings;
    IAdminTokenStore *admin_tokens;   /* may be NULL: then nothing answers its own requests */
} AutomationManagerDeps;

#endif
