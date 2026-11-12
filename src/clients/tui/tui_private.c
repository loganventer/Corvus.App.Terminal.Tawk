/* Private chats on the screen: a private chat is veiled until Touch ID or
 * the lock password lets it open, and locks again as soon as you leave it.
 * The guard decides when something may go ahead; this file only carries
 * out what it lets through, as a switch on what was asked. */
#include "tui_app_state.h"
#include "engines/private_chat_policy.h"
#include "utilities/clock_util.h"

#include <string.h>

static AccountId seen_through(const TuiApp *app, const Chat *chat) {
    return chat && chat->account != ACCOUNT_ID_NONE ? chat->account : app->deps.active_account;
}

int tui_app_private_veiled(const TuiApp *app, const Chat *chat) {
    return private_chat_policy_veiled(chat, seen_through(app, chat), &app->privacy.session);
}

/* What passed is carried out. Nothing reaches here without the guard's DONE. */
static void carry_out(TuiApp *app, const PrivateRequest *done) {
    switch (done->action) {
    case PRIVATE_ACTION_OPEN:            break;
    case PRIVATE_ACTION_MAKE_PRIVATE:    break;
    case PRIVATE_ACTION_MAKE_PUBLIC:     break;
    case PRIVATE_ACTION_EXPORT:          break;
    case PRIVATE_ACTION_CHANGE_PASSWORD: tui_app_toast(app, "\xF0\x9F\x94\x92 Lock password set", 0); break;
    }
    app->dirty = 1;
}

static void follow(TuiApp *app, PrivateGuardOutcome outcome, const PrivateRequest *done) {
    if (outcome == PRIVATE_GUARD_DONE) carry_out(app, done);
    app->dirty = 1;
}

void tui_app_request_private(TuiApp *app, const PrivateRequest *request) {
    PrivateRequest done;
    memset(&done, 0, sizeof(done));
    follow(app, private_chat_guard_request(&app->privacy, request, clock_now_ms()), &done);
}

void tui_app_private_tick(TuiApp *app, int64_t now_ms) {
    if (!private_chat_guard_busy(&app->privacy)) return;
    PrivateRequest done;
    memset(&done, 0, sizeof(done));
    PrivateGuardOutcome outcome = private_chat_guard_tick(&app->privacy, now_ms, &done);
    if (outcome != PRIVATE_GUARD_WAITING) follow(app, outcome, &done);
    if (app->privacy.unlock.state == PRIVATE_UNLOCK_WAIT) app->dirty = 1;        /* the countdown */
}

int tui_app_private_key(TuiApp *app, int is_key, int ch) {
    if (!private_chat_guard_busy(&app->privacy)) return 0;
    PrivateRequest done;
    memset(&done, 0, sizeof(done));
    follow(app, private_chat_guard_key(&app->privacy, is_key, ch, clock_now_ms(), &done), &done);
    return 1;
}

void tui_app_private_render(TuiApp *app, UiRect area) {
    int64_t now = clock_now_ms();
    if (app->privacy.password.open) lock_password_dialog_render(&app->privacy.password, area);
    else private_unlock_dialog_render(&app->privacy.unlock, area, now);
}
