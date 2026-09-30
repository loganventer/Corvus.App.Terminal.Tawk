/* Opening a chat keeps the chat list folded as it was left when asked to
 * (reopening the last chat at startup), and opens the group holding it
 * otherwise (search, next unread, notifications). */
#include "clients/tui/chat_list_view.h"
#include "core/chat.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

static void make(Chat *c, const char *jid, int pinned) {
    memset(c, 0, sizeof(*c));
    snprintf(c->jid, sizeof(c->jid), "%s", jid);
    snprintf(c->name, sizeof(c->name), "%s", jid);
    c->is_pinned = pinned;
}

static int shown(const ChatListView *v, const Chat *chats, const char *jid) {
    for (int n = 0; n < v->entry_count; n++)
        if (v->entries[n].kind == CHAT_LIST_ENTRY_CHAT && strcmp(chats[v->entries[n].chat].jid, jid) == 0) return 1;
    return 0;
}

int main(void) {
    Chat chats[3];
    make(&chats[0], "pinned", 1);
    make(&chats[1], "other", 0);
    make(&chats[2], "another", 0);
    ChatListView v;

    /* Both groups folded, as saved; reopening the last chat must not undo that. */
    chat_list_view_init(&v);
    v.pinned_collapsed = v.others_collapsed = 1;
    chat_list_view_sync(&v, chats, 3);
    chat_list_view_reveal(&v, chats, 3, "other", 0);
    CHECK(v.pinned_collapsed && v.others_collapsed, "reopening the last chat keeps both groups folded");
    CHECK(!shown(&v, chats, "other"), "its folded group stays closed");
    chat_list_view_reveal(&v, chats, 3, "pinned", 0);
    CHECK(v.pinned_collapsed, "the same for a pinned chat");

    /* Going to a chat on purpose opens only the group holding it. */
    chat_list_view_reveal(&v, chats, 3, "other", 1);
    CHECK(!v.others_collapsed && v.pinned_collapsed, "going to an unpinned chat opens only Chats");
    CHECK(shown(&v, chats, "other"), "the chat is then in the list");
    chat_list_view_reveal(&v, chats, 3, "pinned", 1);
    CHECK(!v.pinned_collapsed, "going to a pinned chat opens Pinned");

    if (failures) return 1;
    printf("ok: chat list folding stays as it was left\n");
    return 0;
}
