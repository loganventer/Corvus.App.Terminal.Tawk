#ifndef APP_CORE_AUTOMATION_STATUS_H
#define APP_CORE_AUTOMATION_STATUS_H

#include "core/automation_session.h"

#define AUTOMATION_STATUS_SESSIONS 16

/* What the control socket is doing, for the header and the Agents tab. */
typedef struct AutomationStatus {
    int  listening;
    int  mcp_sessions;       /* connected clients acting for a model */
    int  cli_sessions;
    int  waiting;            /* requests waiting for your answer */
    char socket_path[512];
    char error[160];         /* why it is not listening, when it should be */
    AutomationSession sessions[AUTOMATION_STATUS_SESSIONS];
    int  session_count;
} AutomationStatus;

#endif
