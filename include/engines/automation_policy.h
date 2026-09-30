#ifndef APP_ENGINES_AUTOMATION_POLICY_H
#define APP_ENGINES_AUTOMATION_POLICY_H

#include "core/approval_risk.h"
#include "core/automation_verdict.h"
#include "core/chat.h"
#include "core/control_origin.h"
#include "core/setting_field.h"
#include "core/settings.h"
#include "core/write_kind.h"

/* What control socket clients may see and do, from the Automation settings.
 * Locked chats, soft-locked chats and broadcast feeds are never shown; a
 * non-empty chat list narrows the rest to the chats it names (by name,
 * JID or phone number). */
int               automation_policy_chat_allowed(const Settings *settings, const Chat *chat);
/* Sending needs access "send", anything else "manage". Clients acting for
 * a model always ask you first; your own shell commands ask when
 * confirm_cli is on, and always for destructive writes. */
AutomationVerdict automation_policy_write(const Settings *settings, ControlOrigin origin, WriteKind kind);
/* Whether a setting may be changed over the control socket: never the
 * automation settings, commands, folders, the backend or the log level. */
int               automation_policy_setting_changeable(const SettingField *field);
/* Destructive writes are HIGH, reactions and read marks LOW, everything else MEDIUM. */
ApprovalRisk      automation_policy_risk(const char *op, WriteKind kind);

#endif
