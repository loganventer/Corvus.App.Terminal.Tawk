#include "managers/automation_manager.h"
#include "engines/automation_policy.h"
#include "engines/chat_reference_resolver.h"
#include "engines/confirmation_token.h"
#include "engines/rate_limiter.h"
#include "utilities/log.h"
#include "utilities/str_util.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_COMMANDS 16

struct AutomationManager {
    AutomationManagerDeps deps;
    RateLimiter           writes;
    AutomationStatus      status;
    int                   changed;
    AutomationCommand     commands[MAX_COMMANDS];
    int                   command_count;
};

AutomationManager *automation_manager_create(const AutomationManagerDeps *deps) {
    AutomationManager *m = calloc(1, sizeof(*m));
    if (!m) return NULL;
    m->deps = *deps;
    rate_limiter_init(&m->writes);
    return m;
}

void automation_manager_destroy(AutomationManager *m) { free(m); }

int automation_manager_chat_allowed(AutomationManager *m, const Chat *chat) {
    return automation_policy_chat_allowed(m->deps.settings, chat);
}

ChatResolution automation_manager_resolve(AutomationManager *m, const Chat *chats, int count, const char *ref,
                                          int *found, int *candidates, int max, int *candidate_count) {
    return chat_reference_resolve(chats, count, m->deps.settings, ref, found, candidates, max, candidate_count);
}

const char *automation_manager_access(AutomationManager *m) {
    const char *a = m->deps.settings->automation_access;
    return strcmp(a, "manage") == 0 || strcmp(a, "send") == 0 ? a : "read";
}

int automation_manager_setting_changeable(AutomationManager *m, const SettingField *field) {
    (void)m;
    return automation_policy_setting_changeable(field);
}

AutomationVerdict automation_manager_check_write(AutomationManager *m, ControlOrigin origin, WriteKind kind,
                                                int64_t now_ms, int *retry_after_s) {
    AutomationVerdict verdict = automation_policy_write(m->deps.settings, origin, kind);
    if (verdict == AUTOMATION_VERDICT_REFUSE) return verdict;
    if (!rate_limiter_take(&m->writes, m->deps.settings->automation_rate, now_ms, retry_after_s)) return AUTOMATION_VERDICT_RATE_LIMITED;
    return verdict;
}

ApprovalRisk automation_manager_risk(AutomationManager *m, const char *op, WriteKind kind) {
    (void)m;
    return automation_policy_risk(op, kind);
}

int64_t automation_manager_answer_window_ms(AutomationManager *m, ApprovalRisk risk) {
    (void)m;
    return risk == APPROVAL_RISK_HIGH ? 2 * 60 * 1000 : 5 * 60 * 1000;
}

void automation_manager_command(AutomationManager *m, AutomationCommandKind kind, int conn) {
    if (m->command_count >= MAX_COMMANDS) return;
    m->commands[m->command_count++] = (AutomationCommand){ kind, conn };
}

int automation_manager_take_command(AutomationManager *m, AutomationCommand *out) {
    if (m->command_count == 0) return 0;
    *out = m->commands[0];
    memmove(m->commands, m->commands + 1, (size_t)--m->command_count * sizeof(m->commands[0]));
    return 1;
}

int automation_manager_new_token(AutomationManager *m, char *out, unsigned long size) {
    (void)m;
    return confirmation_token_generate(out, size);
}

void automation_manager_record(AutomationManager *m, ControlOrigin origin, const char *client, const char *op,
                               const char *chat_jid, const char *summary, AutomationOutcome outcome) {
    AutomationEntry e;
    memset(&e, 0, sizeof(e));
    e.at = (int64_t)time(NULL);
    e.origin = origin;
    e.outcome = outcome;
    str_copy(e.client, sizeof(e.client), client ? client : "");
    str_copy(e.op, sizeof(e.op), op ? op : "");
    str_copy(e.chat_jid, sizeof(e.chat_jid), chat_jid ? chat_jid : "");
    str_copy(e.summary, sizeof(e.summary), summary ? summary : "");
    str_strip_controls(e.summary);
    if (m->deps.log && m->deps.log->append(m->deps.log, &e) != 0) LOG_WARN("automation: could not log %s", e.op);
    LOG_INFO("automation: %s %s %s %s", control_origin_name(origin), e.op, e.chat_jid, automation_outcome_name(outcome));
    m->changed = 1;
}

int automation_manager_recent(AutomationManager *m, int limit, AutomationEntry **out, int *count) {
    *out = NULL;
    *count = 0;
    return m->deps.log ? m->deps.log->recent(m->deps.log, limit, out, count) : -1;
}

void automation_manager_set_status(AutomationManager *m, const AutomationStatus *status) {
    if (memcmp(&m->status, status, sizeof(*status)) != 0) m->changed = 1;
    m->status = *status;
}

const AutomationStatus *automation_manager_status(AutomationManager *m) { return &m->status; }

int automation_manager_take_changed(AutomationManager *m) {
    int c = m->changed;
    m->changed = 0;
    return c;
}
