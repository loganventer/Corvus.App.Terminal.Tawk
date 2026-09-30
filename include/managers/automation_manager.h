#ifndef APP_MANAGERS_AUTOMATION_MANAGER_H
#define APP_MANAGERS_AUTOMATION_MANAGER_H

#include <stdint.h>

#include "core/approval_risk.h"
#include "core/automation_command.h"
#include "core/automation_entry.h"
#include "core/automation_status.h"
#include "core/automation_verdict.h"
#include "core/chat.h"
#include "core/chat_resolution.h"
#include "core/control_origin.h"
#include "core/setting_field.h"
#include "core/write_kind.h"
#include "managers/automation_manager_deps.h"

/* The rules for programs that reach tawk through the control socket: which
 * chats they see, whether a write may go ahead now, after you allow it, or
 * not at all, how many writes a minute they get, and the log of what they
 * did. It does nothing itself: the control client carries out what it
 * allows, and reports what happened. */
typedef struct AutomationManager AutomationManager;

AutomationManager *automation_manager_create(const AutomationManagerDeps *deps);
void               automation_manager_destroy(AutomationManager *mgr);

int               automation_manager_chat_allowed(AutomationManager *mgr, const Chat *chat);
/* The one chat `ref` names among `chats`; see chat_reference_resolve. */
ChatResolution    automation_manager_resolve(AutomationManager *mgr, const Chat *chats, int count, const char *ref,
                                             int *found, int *candidates, int max, int *candidate_count);
/* Whether a write may go ahead; each allowed or asked write counts against
 * the rate. *retry_after_s is set when RATE_LIMITED. */
AutomationVerdict automation_manager_check_write(AutomationManager *mgr, ControlOrigin origin, WriteKind kind,
                                                int64_t now_ms, int *retry_after_s);
/* "read", "send" or "manage", as the settings say. */
const char       *automation_manager_access(AutomationManager *mgr);
int               automation_manager_setting_changeable(AutomationManager *mgr, const SettingField *field);

/* How risky a write is: destructive ones HIGH, reactions and read marks LOW, the rest MEDIUM. */
ApprovalRisk automation_manager_risk(AutomationManager *mgr, const char *op, WriteKind kind);
/* How long a request of that risk waits for you before it is declined. */
int64_t      automation_manager_answer_window_ms(AutomationManager *mgr, ApprovalRisk risk);

/* Things you ask for in the Agents tab, handed to the control client. */
void automation_manager_command(AutomationManager *mgr, AutomationCommandKind kind, int conn);
int  automation_manager_take_command(AutomationManager *mgr, AutomationCommand *out);

/* A fresh one-time token for a destructive request waiting to be confirmed. */
int  automation_manager_new_token(AutomationManager *mgr, char *out, unsigned long size);

void automation_manager_record(AutomationManager *mgr, ControlOrigin origin, const char *client, const char *op,
                               const char *chat_jid, const char *summary, AutomationOutcome outcome);
/* Newest first; the caller frees `*out`. */
int  automation_manager_recent(AutomationManager *mgr, int limit, AutomationEntry **out, int *count);

/* What the control socket is doing, reported by the control client and read by the screen. */
void                    automation_manager_set_status(AutomationManager *mgr, const AutomationStatus *status);
const AutomationStatus *automation_manager_status(AutomationManager *mgr);
/* True once after the status or the log changed. */
int                     automation_manager_take_changed(AutomationManager *mgr);

#endif
