/* The list of chats with a switch each: one by one, all at once, and what is kept. */
#include "clients/tui/chat_toggle_dialog.h"
#include "clients/tui/composer_view.h"
#include "clients/tui/toggle_switch.h"
#include "engines/automation_policy.h"
#include "utilities/str_util.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

#define MOM  "27820000001@s.whatsapp.net"
#define WORK "120363000000000001@g.us"

int main(void) {
    ChatToggleDialog d;
    chat_toggle_dialog_open(&d, "Chats", "All chats");
    str_copy(d.row_jid[0], sizeof(d.row_jid[0]), MOM);           /* as the last render listed them */
    str_copy(d.row_jid[1], sizeof(d.row_jid[1]), WORK);
    d.row_count = 2;

    CHECK(!chat_toggle_dialog_all(&d) && !chat_toggle_dialog_is_on(&d, MOM), "everything starts off");
    chat_toggle_dialog_key(&d, 1, KEY_DOWN);
    chat_toggle_dialog_key(&d, 0, ' ');
    CHECK(chat_toggle_dialog_is_on(&d, MOM) && !chat_toggle_dialog_is_on(&d, WORK), "Space switches the highlighted chat on");
    chat_toggle_dialog_key(&d, 0, ' ');
    CHECK(!chat_toggle_dialog_is_on(&d, MOM), "and off again");
    chat_toggle_dialog_key(&d, 0, ' ');

    chat_toggle_dialog_key(&d, 0, 1);                             /* Ctrl+A */
    CHECK(chat_toggle_dialog_all(&d), "Ctrl+A switches every chat on");
    chat_toggle_dialog_key(&d, 1, KEY_DOWN);
    chat_toggle_dialog_key(&d, 0, ' ');
    CHECK(!chat_toggle_dialog_all(&d) && chat_toggle_dialog_is_on(&d, MOM) && !chat_toggle_dialog_is_on(&d, WORK),
          "a switch flipped under All chats turns All off and leaves the ones chosen before");
    chat_toggle_dialog_key(&d, 1, KEY_UP);
    chat_toggle_dialog_key(&d, 1, KEY_UP);
    chat_toggle_dialog_key(&d, 0, ' ');
    CHECK(chat_toggle_dialog_all(&d), "the All chats row is a switch too");

    const char *out[CHAT_TOGGLE_CAPACITY];
    CHECK(chat_toggle_dialog_key(&d, 0, '\n') == POPUP_CHOSEN && !d.open, "Enter saves");
    CHECK(chat_toggle_dialog_chats(&d, out, CHAT_TOGGLE_CAPACITY) == 1 && strcmp(out[0], MOM) == 0, "with the chats that were on");

    chat_toggle_dialog_open(&d, "Chats", "All chats");
    chat_toggle_dialog_set_on(&d, WORK);
    CHECK(chat_toggle_dialog_key(&d, 0, 27) == POPUP_CLOSED && !d.open, "Esc closes without saving");
    char jid[64];
    int fit = 1;
    for (int i = 0; i < CHAT_TOGGLE_CAPACITY + 2; i++) {
        snprintf(jid, sizeof(jid), "2782000%04d@s.whatsapp.net", i);
        fit = chat_toggle_dialog_set_on(&d, jid);
    }
    CHECK(!fit && d.on_count == CHAT_TOGGLE_CAPACITY, "only so many are kept one by one");
    CHECK(strcmp(toggle_switch_text(1), toggle_switch_text(0)) != 0, "the switch looks different on and off");

    /* The rule the list feeds: only chats chosen for it, or all of them. */
    Settings s;
    settings_set_defaults(&s);
    str_copy(s.automation_access, sizeof(s.automation_access), "admin");
    Chat mom, work;
    chat_init(&mom, MOM);
    chat_init(&work, WORK);
    str_copy(mom.name, sizeof(mom.name), "Mom");
    str_copy(work.name, sizeof(work.name), "Work");
    CHECK(automation_policy_self_approval(&s, "send_message", &mom) == SELF_APPROVAL_CHAT_NOT_LISTED, "with none chosen no chat is answered");
    str_copy(s.automation_self_chats, sizeof(s.automation_self_chats), MOM);
    CHECK(automation_policy_self_approval(&s, "send_message", &mom) == SELF_APPROVAL_ALLOW &&
          automation_policy_self_approval(&s, "send_message", &work) == SELF_APPROVAL_CHAT_NOT_LISTED, "a chosen chat is, another is not");
    str_copy(s.automation_self_chats, sizeof(s.automation_self_chats), "*," MOM);
    CHECK(automation_policy_self_approval(&s, "send_message", &work) == SELF_APPROVAL_ALLOW, "All chats answers in every chat");
    str_copy(s.automation_chats, sizeof(s.automation_chats), "Mom");
    CHECK(automation_policy_self_approval(&s, "send_message", &work) == SELF_APPROVAL_CHAT_NOT_LISTED, "but never one agents may not use at all");

    /* The message input's clear button is there only while something is typed. */
    static ComposerView composer;
    composer_view_init(&composer);
    composer.clear_button = (UiRect){ 5, 40, 1, 3 };                /* as drawn with text in the input */
    CHECK(composer_view_hit_clear(&composer, 5, 41) && !composer_view_hit_clear(&composer, 5, 44), "the clear button is where it was drawn");
    composer_view_set_text(&composer, "half a thought");
    CHECK(!composer_view_is_empty(&composer), "typing leaves something to clear");
    composer_view_clear(&composer);
    CHECK(composer_view_is_empty(&composer), "and clearing empties the input");

    if (failures) return 1;
    printf("ok: chats are switched on one by one or all at once, and only those are answered by an agent itself\n");
    return 0;
}
