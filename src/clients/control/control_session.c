#include "clients/control/control_session.h"

#include "utilities/str_util.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

void control_session_init(ControlSession *s, int conn) {
    memset(s, 0, sizeof(*s));
    s->conn = conn;
    s->origin = CONTROL_ORIGIN_MCP;              /* the careful choice until it says otherwise */
    s->since = (int64_t)time(NULL);
}

void control_session_dispose(ControlSession *s) {
    free(s->watches);
    s->watches = NULL;
    s->watch_count = 0;
}

int control_session_follows(const ControlSession *s, const char *jid) {
    if (!s->subscribed) return 0;
    if (s->all_chats) return 1;
    for (int i = 0; i < s->chat_count; i++) if (strcmp(s->chats[i], jid) == 0) return 1;
    return 0;
}

int control_session_allows(const ControlSession *s, const char *op, const char *chat_jid) {
    for (int i = 0; i < s->allowance_count; i++) {
        if (!strcmp(s->allowances[i].op, op) && !strcmp(s->allowances[i].chat_jid, chat_jid)) return 1;
    }
    return 0;
}

void control_session_allow(ControlSession *s, const char *op, const char *chat_jid) {
    if (control_session_allows(s, op, chat_jid) || s->allowance_count >= CONTROL_SESSION_ALLOWANCES) return;
    ControlAllowance *a = &s->allowances[s->allowance_count++];
    str_copy(a->op, sizeof(a->op), op);
    str_copy(a->chat_jid, sizeof(a->chat_jid), chat_jid);
}
