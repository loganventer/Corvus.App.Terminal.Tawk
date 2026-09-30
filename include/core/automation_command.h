#ifndef APP_CORE_AUTOMATION_COMMAND_H
#define APP_CORE_AUTOMATION_COMMAND_H

#include "core/automation_command_kind.h"

/* Something you asked for in the Agents tab about one connected program. */
typedef struct AutomationCommand {
    AutomationCommandKind kind;
    int                   conn;
} AutomationCommand;

#endif
