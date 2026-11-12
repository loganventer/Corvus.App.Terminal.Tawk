/* Private chats on the screen: a private chat is veiled until Touch ID or
 * the lock password lets it open, and locks again as soon as you leave it.
 * The guard decides when something may go ahead; this file only carries
 * out what it lets through, as a switch on what was asked. */
#include "tui_app_state.h"
#include "engines/private_chat_policy.h"
#include "utilities/clock_util.h"
#include "utilities/str_util.h"

#include <string.h>

static AccountId seen_through(const TuiApp *app, const Chat *chat) {
    return chat && chat->account != ACCOUNT_ID_NONE ? chat->account : app->deps.active_account;
}

int tui_app_private_veiled(const TuiApp *app, const Chat *chat) {
    return private_chat_policy_veiled(chat, seen_through(app, chat), &app->privacy.session);
}

static const Chat *find_chat(TuiApp *app, const char *jid) {
    int count = 0;
    const Chat *chats = messaging_manager_chats(app->deps.messaging, &count);
    for (int i = 0; i < count; i++) if (strcmp(chats[i].jid, jid) == 0) return &chats[i];
    return NULL;
}

static PrivateRequest request_for(PrivateAction action, AccountId account, const char *jid) {
    PrivateRequest r;
    memset(&r, 0, sizeof(r));
    r.action = action;
    r.account = account;
    str_copy(r.jid, sizeof(r.jid), jid);
    return r;
}

/* The other accounts a merged row is on. */
static void add_peers(TuiApp *app, const Chat *row, PrivateRequest *r) {
    IAccountDirectory *dir = app->deps.directory;
    if (!dir || !(row->accounts & (row->accounts - 1))) return;
    for (int i = 0; i < ACCOUNT_MAX && r->peer_count < ACCOUNT_MAX; i++) {
        if (!(row->accounts & (1u << i))) continue;
        const AccountServices *sv = dir->at(dir, i);
        if (!sv || sv->id == r->account) continue;
        r->peers[r->peer_count].account = sv->id;
        str_copy(r->peers[r->peer_count].jid, sizeof(r->peers[r->peer_count].jid), row->jid);
        r->peer_count++;
    }
}

int tui_app_private_gate_open(TuiApp *app, const char *jid) {
    const Chat *chat = find_chat(app, jid);
    AccountId account = app->deps.active_account;
    if (!private_chat_policy_needs_unlock(UNLOCK_FOR_OPEN, chat, account, &app->privacy.session)) return 0;
    memset(&app->private_row, 0, sizeof(app->private_row));
    PrivateRequest r = request_for(PRIVATE_ACTION_OPEN, account, jid);
    tui_app_request_private(app, &r);
    return 1;
}

int tui_app_private_gate_row(TuiApp *app, const Chat *row) {
    AccountId account = seen_through(app, row);
    if (!private_chat_policy_needs_unlock(UNLOCK_FOR_OPEN, row, account, &app->privacy.session)) return 0;
    app->private_row = *row;
    PrivateRequest r = request_for(PRIVATE_ACTION_OPEN, account, row->jid);
    add_peers(app, row, &r);
    tui_app_request_private(app, &r);
    return 1;
}

/* Nothing of a chat that just locked stays on screen: no selected message, no picture. */
static void forget_unlocked(TuiApp *app) {
    app->message_view.selected = -1;
    if (app->viewer.open) image_viewer_close(&app->viewer);
    app->contact.open = 0;
    sixel_overlay_invalidate(&app->sixel_overlay);
    app->dirty = 1;
}

int tui_app_private_gate_export(TuiApp *app, const char *jid, int with_media) {
    const Chat *chat = find_chat(app, jid);
    AccountId account = app->deps.active_account;
    if (!private_chat_policy_needs_unlock(UNLOCK_FOR_EXPORT, chat, account, &app->privacy.session)) return 0;
    PrivateRequest r = request_for(PRIVATE_ACTION_EXPORT, account, jid);
    r.with_media = with_media;
    tui_app_request_private(app, &r);
    return 1;
}

void tui_app_leave_private(TuiApp *app, AccountId account, const char *jid) {
    int had = app->privacy.session.count > 0;
    private_chat_guard_leave(&app->privacy, account, jid);
    if (had && app->privacy.session.count == 0) forget_unlocked(app);
}

void tui_app_private_relock(TuiApp *app) {
    int had = app->privacy.session.count > 0 || private_chat_guard_busy(&app->privacy);
    private_chat_guard_relock_all(&app->privacy);
    memset(&app->private_row, 0, sizeof(app->private_row));
    if (had) forget_unlocked(app);
}

/* The chat passed: open it, and its other accounts' sides when it came from a merged row. */
static void open_unlocked(TuiApp *app, const PrivateRequest *done) {
    char jid[128];
    str_copy(jid, sizeof(jid), done->jid);
    Chat row = app->private_row;
    memset(&app->private_row, 0, sizeof(app->private_row));
    tui_app_open_chat_now(app, jid, 1);
    if (!strcmp(row.jid, jid)) tui_app_open_peers(app, &row);
}

/* The chat list row of `jid` as seen through `account`, which says which accounts a merged chat is on. */
static const Chat *row_of(TuiApp *app, AccountId account, const char *jid) {
    int count = 0;
    const Chat *rows = tui_app_chat_rows(app, &count);
    for (int i = 0; i < count; i++) {
        if (strcmp(rows[i].jid, jid) != 0) continue;
        if (rows[i].account == ACCOUNT_ID_NONE || rows[i].account == account || (rows[i].accounts & (rows[i].accounts - 1))) return &rows[i];
    }
    return NULL;
}

void tui_app_set_private(TuiApp *app, const char *jid, int on) {
    if (!jid || !jid[0]) { tui_app_toast(app, "Select or open a chat first", 1); return; }
    char copy[128];
    str_copy(copy, sizeof(copy), jid);
    const Chat *chat = find_chat(app, copy);
    if (!chat || (chat->is_private != 0) == (on != 0)) return;
    PrivateRequest r = request_for(on ? PRIVATE_ACTION_MAKE_PRIVATE : PRIVATE_ACTION_MAKE_PUBLIC, app->deps.active_account, copy);
    const Chat *row = row_of(app, app->deps.active_account, copy);
    if (row) add_peers(app, row, &r);
    tui_app_request_private(app, &r);
}

void tui_app_toggle_private(TuiApp *app, const char *jid) {
    const Chat *chat = jid && jid[0] ? find_chat(app, jid) : NULL;
    tui_app_set_private(app, jid, chat ? !chat->is_private : 1);
}

void tui_app_toggle_private_here(TuiApp *app) {
    const char *jid = messaging_manager_open_jid(app->deps.messaging);
    if (app->focus == TUI_FOCUS_CHATS) {
        const char *selected = tui_app_take_selected(app);
        if (selected) jid = selected;
    }
    tui_app_toggle_private(app, jid);
}

void tui_app_change_lock_password(TuiApp *app) {
    PrivateRequest r = request_for(PRIVATE_ACTION_CHANGE_PASSWORD, app->deps.active_account, "");
    tui_app_request_private(app, &r);
}

/* The change goes to every account the chat is on; a chat made private locks at once. */
static void apply_private(TuiApp *app, const PrivateRequest *done, int on) {
    messaging_manager_set_private(app->deps.messaging, done->jid, on);
    IAccountDirectory *dir = app->deps.directory;
    for (int i = 0; dir && i < done->peer_count; i++) {
        const AccountServices *sv = dir->find(dir, done->peers[i].account);
        if (sv) messaging_manager_set_private(sv->messaging, done->peers[i].jid, on);
    }
    if (on) tui_app_private_relock(app);
    app->chat_rows_stale = 1;
    tui_app_toast(app, on ? "\xF0\x9F\x94\x92 Private: it opens with Touch ID or the lock password" : "Public again", 0);
}

/* What passed is carried out. Nothing reaches here without the guard's DONE. */
static void carry_out(TuiApp *app, const PrivateRequest *done) {
    switch (done->action) {
    case PRIVATE_ACTION_OPEN:            open_unlocked(app, done); break;
    case PRIVATE_ACTION_MAKE_PRIVATE:    apply_private(app, done, 1); break;
    case PRIVATE_ACTION_MAKE_PUBLIC:     apply_private(app, done, 0); break;
    case PRIVATE_ACTION_EXPORT:          tui_app_export_chat_now(app, done->jid, done->with_media); break;
    case PRIVATE_ACTION_CHANGE_PASSWORD: tui_app_toast(app, "\xF0\x9F\x94\x92 The lock password for private chats is set", 0); break;
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
    follow(app, private_chat_guard_request(&app->privacy, request, clock_now_ms(), &done), &done);
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
