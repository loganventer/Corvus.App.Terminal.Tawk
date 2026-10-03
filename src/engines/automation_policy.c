#include "engines/automation_policy.h"
#include "engines/chat_match.h"
#include "engines/chat_visibility.h"

#include <ctype.h>
#include <string.h>
#include <strings.h>

/* Digits only, so "+27 82 000 0000" and "27820000000@s.whatsapp.net" compare equal. */
static void digits_of(const char *s, size_t n, char *out, size_t size) {
    size_t k = 0;
    for (size_t i = 0; i < n && s[i] && s[i] != '@' && k + 1 < size; i++) {
        if (isdigit((unsigned char)s[i])) out[k++] = s[i];
    }
    out[k] = '\0';
}

static int names_chat(const char *item, size_t n, const Chat *c) {
    if (n == 0) return 0;
    if (strlen(c->jid) == n && strncmp(c->jid, item, n) == 0) return 1;
    if (strlen(c->name) == n && strncasecmp(c->name, item, n) == 0) return 1;
    char want[64], have[64];
    digits_of(item, n, want, sizeof(want));
    digits_of(c->jid, strlen(c->jid), have, sizeof(have));
    return strlen(want) >= 6 && strcmp(want, have) == 0;
}

static int listed(const char *list, const Chat *c) {
    const char *p = list;
    while (*p) {
        while (*p == ',' || isspace((unsigned char)*p)) p++;
        const char *end = strchr(p, ',');
        size_t n = end ? (size_t)(end - p) : strlen(p);
        while (n > 0 && isspace((unsigned char)p[n - 1])) n--;
        if (names_chat(p, n, c)) return 1;
        if (!end) break;
        p = end + 1;
    }
    return 0;
}

int automation_policy_chat_allowed(const Settings *s, const Chat *c) {
    if (!c || chat_visibility_hidden(c->jid) || !chat_match_searchable(c, 0) || c->soft_locked) return 0;
    const char *list = s->automation_chats;
    while (isspace((unsigned char)*list)) list++;
    return *list ? listed(list, c) : 1;
}

static int access_level(const char *access) {
    if (strcmp(access, "admin") == 0) return 3;
    if (strcmp(access, "manage") == 0) return 2;
    if (strcmp(access, "send") == 0) return 1;
    return 0;
}

AutomationVerdict automation_policy_write(const Settings *s, ControlOrigin origin, WriteKind kind) {
    int needed = kind == WRITE_KIND_SEND ? 1 : 2;
    if (access_level(s->automation_access) < needed) return AUTOMATION_VERDICT_REFUSE;
    if (origin == CONTROL_ORIGIN_MCP || kind == WRITE_KIND_DESTRUCTIVE) return AUTOMATION_VERDICT_ASK;
    return s->automation_confirm_cli ? AUTOMATION_VERDICT_ASK : AUTOMATION_VERDICT_ALLOW;
}

SelfApprovalVerdict automation_policy_self_approval(const Settings *s, const char *op, const Chat *chat) {
    static const char *const OWN[] = {
        "send_message", "reply_status", "forward_message", "edit_message", "retry_message",
        "schedule_message", "reschedule", "send_scheduled_now", "cancel_scheduled",
        "react", "mark_read", "like_status", NULL
    };
    if (access_level(s->automation_access) < 3) return SELF_APPROVAL_OFF;
    int own = 0;
    for (int i = 0; op && OWN[i] && !own; i++) own = strcmp(op, OWN[i]) == 0;
    if (!own) return SELF_APPROVAL_NOT_THIS_KIND;
    const char *list = s->automation_chats;
    while (isspace((unsigned char)*list)) list++;
    if (!chat || !*list || !automation_policy_chat_allowed(s, chat)) return SELF_APPROVAL_CHAT_NOT_LISTED;
    return SELF_APPROVAL_ALLOW;
}

int automation_policy_setting_changeable(const SettingField *f) {
    static const char *const FIXED[] = {
        "command", "image_viewer", "video_player", "node_binary", "sidecar_dir",       /* run programs */
        "data_dir", "media_dir", "download_dir", "attach_dir", "sound_file",          /* folders and files */
        "backend", "log_level", "last_chat", "recent_emoji", NULL
    };
    if (!f || f->category == SETTING_CATEGORY_AUTOMATION || f->category == SETTING_CATEGORY_ADVANCED) return 0;
    for (int i = 0; FIXED[i]; i++) if (strcmp(f->key, FIXED[i]) == 0) return 0;
    return 1;
}

ApprovalRisk automation_policy_risk(const char *op, WriteKind kind) {
    if (kind == WRITE_KIND_DESTRUCTIVE) return APPROVAL_RISK_HIGH;
    if (op && (strcmp(op, "react") == 0 || strcmp(op, "mark_read") == 0 || strcmp(op, "like_status") == 0)) return APPROVAL_RISK_LOW;
    return APPROVAL_RISK_MEDIUM;
}
