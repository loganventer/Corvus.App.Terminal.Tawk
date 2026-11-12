/* Making a chat private and public: the first time asks for a lock password,
 * making one public asks for an unlock, and a merged chat changes on every
 * account it is on. */
#include "tui_harness.h"

static int failures;
#define CHECK(cond, what) do { if (!(cond)) { printf("FAIL: %s\n", what); failures++; } } while (0)

#define DIARY "27820000001@s.whatsapp.net"

static int is_private_on(Harness *h, int index) {
    int count = 0;
    const Chat *chats = messaging_manager_chats(h->accounts[index].messaging, &count);
    for (int i = 0; i < count; i++) if (!strcmp(chats[i].jid, DIARY)) return chats[i].is_private;
    return -1;
}

static void keys(Harness *h, const char *text) {
    for (const char *p = text; *p; p++) tui_app_private_key(h->app, 0, (unsigned char)*p);
}

static void first_password_then_public(void) {
    Harness h;
    if (harness_open(&h, 0, NULL) != 0) { printf("FAIL: the harness opens\n"); failures++; return; }
    harness_chat(&h, 0, DIARY, "Diary", 0);
    tui_app_set_private(h.app, DIARY, 1);
    CHECK(h.app->privacy.password.open && h.app->privacy.password.first && is_private_on(&h, 0) == 0,
          "with no lock password yet, making a chat private asks for one first");
    keys(&h, "abcd\rabcd\r");
    CHECK(is_private_on(&h, 0) == 1 && privacy_manager_has_password(h.privacy.manager) && !private_chat_guard_busy(&h.app->privacy),
          "once it is set twice the chat is private");

    tui_app_set_private(h.app, DIARY, 0);
    CHECK(h.app->privacy.unlock.open && is_private_on(&h, 0) == 1, "making it public asks for an unlock first");
    keys(&h, "\x1b");
    CHECK(is_private_on(&h, 0) == 1, "and without one it stays private");
    tui_app_set_private(h.app, DIARY, 0);
    keys(&h, "nope\r");
    CHECK(is_private_on(&h, 0) == 1, "a wrong password leaves it private");
    keys(&h, "abcd\r");
    CHECK(is_private_on(&h, 0) == 0, "the right one makes it public");
    harness_close(&h);
}

static void merged_chat(void) {
    Harness h;
    if (harness_open(&h, 0, "letmein") != 0) { printf("FAIL: the harness opens\n"); failures++; return; }
    harness_chat(&h, 0, DIARY, "Diary", 0);
    harness_chat(&h, 1, DIARY, "Diary", 0);
    h.app->chat_rows_stale = 1;
    tui_app_set_private(h.app, DIARY, 1);
    CHECK(is_private_on(&h, 0) == 1 && is_private_on(&h, 1) == 1, "with a password set, a merged chat is made private on both accounts at once");
    tui_app_set_private(h.app, DIARY, 0);
    keys(&h, "letmein\r");
    CHECK(is_private_on(&h, 0) == 0 && is_private_on(&h, 1) == 0, "and made public on both");
    harness_close(&h);
}

int main(void) {
    first_password_then_public();
    merged_chat();
    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: a chat is made private after a lock password exists, public only after an unlock, on every account it is merged across\n");
    return 0;
}
