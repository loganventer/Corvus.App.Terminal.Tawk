/* What a private chat shows: veiled while locked (the conversation, its
 * subtitle and, with the messages, its transcripts and TL;DR), as it is
 * once unlocked, and veiled again when it locks. */
#include "clients/tui/chat_subtitle.h"
#include "tui_harness.h"

static int failures;
#define CHECK(cond, what) do { if (!(cond)) { printf("FAIL: %s\n", what); failures++; } } while (0)

#define DIARY "27820000001@s.whatsapp.net"
#define MOM   "27820000002@s.whatsapp.net"

static int pane_veiled(Harness *h) {
    return tui_app_private_veiled(h->app, messaging_manager_open_chat_info(h->app->deps.messaging));
}

int main(void) {
    Harness h;
    if (harness_open(&h, 0, "letmein") != 0) { printf("FAIL: the harness opens\n"); return 1; }
    harness_chat(&h, 0, DIARY, "Diary", 0);
    harness_chat(&h, 0, MOM, "Mom", 0);

    tui_app_open_chat(h.app, DIARY);
    CHECK(!pane_veiled(&h), "a public chat's pane is not veiled");
    messaging_manager_set_private(h.app->deps.messaging, DIARY, 1);     /* made private while it is open */
    CHECK(pane_veiled(&h), "a locked private chat's pane is veiled");
    const Chat *open = messaging_manager_open_chat_info(h.app->deps.messaging);
    char out[96] = "x";
    chat_subtitle_text(settings_manager_current(h.settings), open, pane_veiled(&h), "about text", 0, out, sizeof(out));
    CHECK(out[0] == '\0', "and its subtitle says nothing");

    tui_app_open_chat(h.app, MOM);
    tui_app_open_chat(h.app, DIARY);
    harness_type(&h, "letmein");
    CHECK(!pane_veiled(&h), "once unlocked the pane is clear");
    open = messaging_manager_open_chat_info(h.app->deps.messaging);
    chat_subtitle_text(settings_manager_current(h.settings), open, pane_veiled(&h), "about text", 0, out, sizeof(out));
    CHECK(!strcmp(out, "about text"), "and its subtitle shows again");

    tui_app_private_relock(h.app);
    CHECK(pane_veiled(&h), "locking it again veils it again");

    harness_close(&h);
    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: a private chat is veiled while locked, shown once unlocked, and veiled again when it locks\n");
    return 0;
}
