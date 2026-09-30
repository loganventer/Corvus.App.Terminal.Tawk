/* Dragging chats into and out of the Pinned group in the chat list. */
#include "clients/tui/chat_list_view.h"
#include "core/chat.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

/* Two rows per entry (detailed list, no spacing), starting at row 0. */
static const UiRect RECT = { 0, 0, 40, 30 };
static int row_of(int entry) { return entry * 2; }

static void make(Chat *c, const char *jid, int pinned) {
    memset(c, 0, sizeof(*c));
    snprintf(c->jid, sizeof(c->jid), "%s", jid);
    snprintf(c->name, sizeof(c->name), "%s", jid);
    c->is_pinned = pinned;
}

static int entry_of(const ChatListView *v, const Chat *chats, const char *jid) {
    for (int n = 0; n < v->entry_count; n++)
        if (v->entries[n].kind == CHAT_LIST_ENTRY_CHAT && strcmp(chats[v->entries[n].chat].jid, jid) == 0) return n;
    return -1;
}

/* Presses on `jid`, moves to row `to` and releases there; returns what drag_end says. */
static int drag(ChatListView *v, const Chat *chats, int count, const char *jid, int to, char *out) {
    v->selected = entry_of(v, chats, jid);
    if (!chat_list_view_drag_begin(v, chats)) return -2;
    chat_list_view_drag_move(v, chats, count, RECT, to);
    return chat_list_view_drag_end(v, chats, count, RECT, to, out, 128);
}

int main(void) {
    ChatListView v;
    char jid[128];

    /* Pinned: a. Others: b, c.  Entries: [Pinned] a [Chats] b c */
    Chat chats[3];
    make(&chats[0], "a", 1);
    make(&chats[1], "b", 0);
    make(&chats[2], "c", 0);
    chat_list_view_init(&v);
    chat_list_view_sync(&v, chats, 3);
    CHECK(v.entry_count == 5 && v.entries[0].kind == CHAT_LIST_ENTRY_PINNED && v.entries[2].kind == CHAT_LIST_ENTRY_OTHERS,
          "the list has a Pinned and a Chats group");

    jid[0] = '\0';
    CHECK(drag(&v, chats, 3, "b", row_of(0), jid) == 1 && strcmp(jid, "b") == 0, "dropping b on the Pinned header pins it");
    jid[0] = '\0';
    CHECK(drag(&v, chats, 3, "c", row_of(1), jid) == 1 && strcmp(jid, "c") == 0, "dropping c on a pinned chat pins it");
    jid[0] = '\0';
    CHECK(drag(&v, chats, 3, "a", row_of(2), jid) == 0 && strcmp(jid, "a") == 0, "dropping a on the Chats header unpins it");
    CHECK(drag(&v, chats, 3, "b", row_of(entry_of(&v, chats, "b")), jid) == -1, "a click without moving changes nothing");
    CHECK(drag(&v, chats, 3, "b", row_of(4), jid) == -1, "dropping b among unpinned chats changes nothing");
    CHECK(!v.dragging && !v.drag_jid[0], "the drag is over after the release");

    /* Nothing pinned yet: the list has no groups until a drag starts. */
    make(&chats[0], "a", 0);
    chat_list_view_init(&v);
    chat_list_view_sync(&v, chats, 3);
    CHECK(v.entry_count == 3 && v.entries[0].kind == CHAT_LIST_ENTRY_CHAT, "without pins there are no groups");
    v.selected = entry_of(&v, chats, "b");
    CHECK(chat_list_view_drag_begin(&v, chats), "a drag can start");
    chat_list_view_drag_move(&v, chats, 3, RECT, row_of(2));
    CHECK(v.dragging && v.entries[0].kind == CHAT_LIST_ENTRY_PINNED, "dragging shows an empty Pinned group");
    CHECK(v.entries[v.selected].kind == CHAT_LIST_ENTRY_CHAT && strcmp(chats[v.entries[v.selected].chat].jid, "b") == 0,
          "the dragged chat stays selected");
    chat_list_view_drag_move(&v, chats, 3, RECT, row_of(0));
    CHECK(v.drop_pinned == 1, "the Pinned header is the drop target");
    jid[0] = '\0';
    CHECK(chat_list_view_drag_end(&v, chats, 3, RECT, row_of(0), jid, sizeof(jid)) == 1 && strcmp(jid, "b") == 0,
          "the first chat can be pinned by dragging");
    CHECK(v.entry_count == 3, "the empty Pinned group goes away again");

    /* Search results and other folders have no groups to drop into. */
    snprintf(v.filter, sizeof(v.filter), "b");
    chat_list_view_sync(&v, chats, 3);
    v.selected = 0;
    CHECK(!chat_list_view_drag_begin(&v, chats), "no dragging in search results");

    if (failures) return 1;
    printf("ok: chats drag into and out of the Pinned group\n");
    return 0;
}
