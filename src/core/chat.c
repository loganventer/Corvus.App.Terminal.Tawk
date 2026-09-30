#include "core/chat.h"
#include "utilities/str_util.h"

#include <string.h>

void chat_init(Chat *chat, const char *jid) {
    memset(chat, 0, sizeof(*chat));
    str_copy(chat->jid, sizeof(chat->jid), jid);
    chat->is_group = chat_jid_is_group(jid);
    chat->is_archived = -1;
    chat->is_locked = -1;
}

int chat_jid_is_group(const char *jid) {
    size_t n = jid ? strlen(jid) : 0;
    return n > 5 && strcmp(jid + n - 5, "@g.us") == 0;
}

int chat_compare(const void *a, const void *b) {
    const Chat *x = a, *y = b;
    if (x->is_pinned != y->is_pinned) return y->is_pinned - x->is_pinned;
    if (x->last_ts != y->last_ts) return x->last_ts < y->last_ts ? 1 : -1;
    return strcmp(x->name, y->name);
}
