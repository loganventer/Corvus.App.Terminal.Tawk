/* A private chat locks again when you leave it, when the screensaver
 * starts, when tawk locks, when you switch account and when tawk quits;
 * and nothing of it stays on screen. */
#include "tui_harness.h"

static int failures;
#define CHECK(cond, what) do { if (!(cond)) { printf("FAIL: %s\n", what); failures++; } } while (0)

#define DIARY "27820000001@s.whatsapp.net"
#define MOM   "27820000002@s.whatsapp.net"

static Chat diary(void) {
    Chat c;
    chat_init(&c, DIARY);
    c.is_private = 1;
    return c;
}

static void unlock(Harness *h) {
    if (h->app->deps.active_account != 1) tui_app_use_account(h->app, 1);
    tui_app_open_chat(h->app, DIARY);
    harness_type(h, "letmein");
    h->app->message_view.selected = 3;                    /* a message picked in it */
}

static int nothing_unlocked(Harness *h) {
    Chat c = diary();
    return h->app->privacy.session.count == 0 && tui_app_private_veiled(h->app, &c) && h->app->message_view.selected == -1;
}

int main(void) {
    Harness h;
    if (harness_open(&h, 0, "letmein") != 0) { printf("FAIL: the harness opens\n"); return 1; }
    harness_chat(&h, 0, DIARY, "Diary", 1);
    harness_chat(&h, 0, MOM, "Mom", 0);

    unlock(&h);
    Chat c = diary();
    CHECK(h.app->privacy.session.count == 1 && !tui_app_private_veiled(h.app, &c), "unlocked, it shows as it is");
    tui_app_open_chat(h.app, MOM);
    CHECK(nothing_unlocked(&h), "leaving it for another chat locks it and forgets the picked message");

    unlock(&h);
    tui_app_use_account(h.app, 2);
    CHECK(nothing_unlocked(&h), "switching account locks it");

    /* The screensaver, the app lock and quitting each call this on their way in (tui_app.c run_screensaver,
     * tui_lock.c tui_app_lock, the end of tui_app_run); running them needs a terminal and an encrypted database. */
    unlock(&h);
    tui_app_private_relock(h.app);
    CHECK(nothing_unlocked(&h), "the screensaver, the app lock and quitting lock it");
    CHECK(!strcmp(harness_open_jid(&h), DIARY), "the chat stays where it was, veiled");

    tui_app_open_chat(h.app, MOM);
    tui_app_open_chat(h.app, DIARY);
    CHECK(private_chat_guard_busy(&h.app->privacy), "an unlock being asked for");
    tui_app_private_relock(h.app);
    CHECK(!private_chat_guard_busy(&h.app->privacy) && strcmp(harness_open_jid(&h), DIARY) != 0, "is dropped too, and opens nothing");

    harness_close(&h);
    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: a private chat locks again on leaving, the screensaver, the app lock, an account switch and quit\n");
    return 0;
}
