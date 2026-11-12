/* Asking for a private chat: Touch ID, the lock password when it is turned
 * down or missing, and Esc. Nothing is carried out without a pass. */
#include "clients/tui/private_chat_guard.h"
#include "private_fakes.h"

#include <ncurses.h>

static int failures;
#define CHECK(cond, what) do { if (!(cond)) { printf("FAIL: %s\n", what); failures++; } } while (0)

#define A "27820000001@s.whatsapp.net"

static PrivateRequest open_request(void) {
    PrivateRequest r;
    memset(&r, 0, sizeof(r));
    r.action = PRIVATE_ACTION_OPEN;
    r.account = 1;
    snprintf(r.jid, sizeof(r.jid), "%s", A);
    r.peers[0] = (UnlockedChat){ 2, A };
    r.peer_count = 1;
    return r;
}

static PrivateGuardOutcome type(PrivateChatGuard *g, const char *text, int64_t now, PrivateRequest *done) {
    PrivateGuardOutcome o = PRIVATE_GUARD_WAITING;
    for (const char *p = text; *p && o == PRIVATE_GUARD_WAITING; p++) o = private_chat_guard_key(g, 0, (unsigned char)*p, now, done);
    return o;
}

static void touch_id_passes(void) {
    FakePrivacy p;
    fake_privacy_init(&p, 1, "letmein");
    PrivateChatGuard g;
    private_chat_guard_init(&g, p.manager);
    PrivateRequest r = open_request(), done;
    memset(&done, 0, sizeof(done));
    CHECK(private_chat_guard_request(&g, &r, 0) == PRIVATE_GUARD_WAITING && p.touch.begun == 1 && private_chat_guard_busy(&g),
          "opening a private chat puts Touch ID up and opens nothing yet");
    CHECK(private_chat_guard_tick(&g, 16, &done) == PRIVATE_GUARD_WAITING && !private_chat_session_is_unlocked(&g.session, 1, A),
          "while the sheet is up nothing is unlocked");
    CHECK(type(&g, "hello", 20, &done) == PRIVATE_GUARD_WAITING && private_chat_guard_busy(&g) && g.unlock.field.length == 0,
          "keys typed meanwhile stay in the dialog and go nowhere");
    p.touch.next = BIOMETRIC_OK;
    CHECK(private_chat_guard_tick(&g, 32, &done) == PRIVATE_GUARD_DONE && done.action == PRIVATE_ACTION_OPEN && !strcmp(done.jid, A),
          "Touch ID OK lets the chat open");
    CHECK(private_chat_session_is_unlocked(&g.session, 1, A) && private_chat_session_is_unlocked(&g.session, 2, A) && !private_chat_guard_busy(&g),
          "and unlocks it on every account it is merged across");
    fake_privacy_dispose(&p);
}

static void fallback_then_password(void) {
    FakePrivacy p;
    fake_privacy_init(&p, 1, "letmein");
    PrivateChatGuard g;
    private_chat_guard_init(&g, p.manager);
    PrivateRequest r = open_request(), done;
    private_chat_guard_request(&g, &r, 0);
    p.touch.next = BIOMETRIC_FALLBACK;
    CHECK(private_chat_guard_tick(&g, 16, &done) == PRIVATE_GUARD_WAITING && g.unlock.state == PRIVATE_UNLOCK_PASSWORD,
          "'Use tawk password' moves to the password");
    CHECK(type(&g, "wrong", 20, &done) == PRIVATE_GUARD_WAITING &&
          private_chat_guard_key(&g, 0, '\r', 20, &done) == PRIVATE_GUARD_WAITING && g.unlock.wrong,
          "a wrong password opens nothing");
    CHECK(!private_chat_session_is_unlocked(&g.session, 1, A), "and leaves it locked");
    type(&g, "letmein", 30, &done);
    CHECK(private_chat_guard_key(&g, 0, '\r', 30, &done) == PRIVATE_GUARD_DONE && done.action == PRIVATE_ACTION_OPEN &&
          private_chat_session_is_unlocked(&g.session, 1, A), "the right one opens it");

    private_chat_guard_relock_all(&g);
    p.touch.available = 0;
    CHECK(private_chat_guard_request(&g, &r, 40) == PRIVATE_GUARD_WAITING && g.unlock.state == PRIVATE_UNLOCK_PASSWORD && p.touch.begun == 1,
          "without Touch ID the password is asked at once");
    p.touch.available = 1;
    private_chat_guard_relock_all(&g);
    private_chat_guard_request(&g, &r, 50);
    CHECK(private_chat_guard_key(&g, 0, '\t', 50, &done) == PRIVATE_GUARD_WAITING && g.unlock.state == PRIVATE_UNLOCK_PASSWORD && p.touch.cancelled >= 1,
          "Tab takes the sheet down and asks for the password");
    fake_privacy_dispose(&p);
}

static void esc_and_cancel(void) {
    FakePrivacy p;
    fake_privacy_init(&p, 1, "letmein");
    PrivateChatGuard g;
    private_chat_guard_init(&g, p.manager);
    PrivateRequest r = open_request(), done;
    memset(&done, 0, sizeof(done));
    private_chat_guard_request(&g, &r, 0);
    CHECK(private_chat_guard_key(&g, 0, 27, 10, &done) == PRIVATE_GUARD_CANCELLED && !private_chat_guard_busy(&g) &&
          !private_chat_session_is_unlocked(&g.session, 1, A) && p.touch.cancelled == 1, "Esc leaves it locked and takes the sheet down");
    private_chat_guard_request(&g, &r, 20);
    p.touch.next = BIOMETRIC_CANCELLED;
    CHECK(private_chat_guard_tick(&g, 30, &done) == PRIVATE_GUARD_CANCELLED && !private_chat_session_is_unlocked(&g.session, 1, A),
          "Cancel on the sheet does the same");
    private_chat_guard_request(&g, &r, 40);
    p.touch.next = BIOMETRIC_FALLBACK;
    private_chat_guard_tick(&g, 50, &done);
    type(&g, "letme", 60, &done);
    CHECK(private_chat_guard_key(&g, 0, 27, 60, &done) == PRIVATE_GUARD_CANCELLED && g.unlock.field.length == 0 &&
          !private_chat_session_is_unlocked(&g.session, 1, A), "Esc in the password field drops what was typed");
    fake_privacy_dispose(&p);
}

int main(void) {
    touch_id_passes();
    fallback_then_password();
    esc_and_cancel();
    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: a private chat opens only after Touch ID or its password, and Esc leaves it locked\n");
    return 0;
}
