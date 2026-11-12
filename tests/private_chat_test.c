/* Private chats: which chat is unlocked, and when it locks again. */
#include "engines/private_chat_policy.h"
#include "engines/private_chat_session.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(cond, what) do { if (!(cond)) { printf("FAIL: %s\n", what); failures++; } } while (0)

#define A "27820000001@s.whatsapp.net"
#define B "27820000002@s.whatsapp.net"

static void session_rules(void) {
    PrivateChatSession s;
    memset(&s, 0, sizeof(s));
    CHECK(!private_chat_session_is_unlocked(&s, 1, A), "nothing is unlocked at first");

    private_chat_session_unlock(&s, 1, A);
    CHECK(private_chat_session_is_unlocked(&s, 1, A), "an unlocked chat is unlocked");
    CHECK(!private_chat_session_is_unlocked(&s, 2, A), "but only on its own account");

    private_chat_session_leave(&s, 1, A);
    CHECK(private_chat_session_is_unlocked(&s, 1, A), "opening the same chat again keeps it unlocked");
    private_chat_session_leave(&s, 1, B);
    CHECK(!private_chat_session_is_unlocked(&s, 1, A), "opening another chat locks it");

    private_chat_session_unlock(&s, 1, A);
    private_chat_session_unlock(&s, 1, B);
    CHECK(!private_chat_session_is_unlocked(&s, 1, A) && private_chat_session_is_unlocked(&s, 1, B), "only one chat is unlocked at a time");

    private_chat_session_unlock(&s, 1, A);
    private_chat_session_add_peer(&s, 2, A);
    CHECK(private_chat_session_is_unlocked(&s, 2, A), "a merged peer is unlocked with its chat");
    private_chat_session_leave(&s, 2, A);
    CHECK(private_chat_session_is_unlocked(&s, 1, A), "moving to the peer's side keeps the chat unlocked");
    private_chat_session_add_peer(&s, 2, A);
    CHECK(s.count == 2, "a peer is kept once");

    private_chat_session_relock_all(&s);
    CHECK(s.count == 0 && !private_chat_session_is_unlocked(&s, 1, A) && !private_chat_session_is_unlocked(&s, 2, A), "relock_all clears everything");
    private_chat_session_add_peer(&s, 2, A);
    CHECK(s.count == 0, "a peer cannot unlock anything on its own");
}

static void policy_rules(void) {
    PrivateChatSession s;
    memset(&s, 0, sizeof(s));
    Chat secret, open;
    chat_init(&secret, A);
    chat_init(&open, B);
    secret.is_private = 1;

    CHECK(private_chat_policy_veiled(&secret, 1, &s), "a locked private chat is veiled");
    CHECK(!private_chat_policy_veiled(&open, 1, &s), "a public chat never is");
    CHECK(!private_chat_policy_tui_searchable(&secret, 1, &s) && private_chat_policy_tui_searchable(&open, 1, &s),
          "search leaves out a locked private chat and keeps a public one");
    CHECK(private_chat_policy_needs_unlock(UNLOCK_FOR_OPEN, &secret, 1, &s), "opening a locked private chat asks first");
    CHECK(!private_chat_policy_needs_unlock(UNLOCK_FOR_OPEN, &open, 1, &s) && !private_chat_policy_needs_unlock(UNLOCK_FOR_EXPORT, &open, 1, &s) &&
          !private_chat_policy_needs_unlock(UNLOCK_FOR_MAKE_PUBLIC, &open, 1, &s), "a public chat never asks");

    private_chat_session_unlock(&s, 1, A);
    CHECK(!private_chat_policy_veiled(&secret, 1, &s), "an unlocked private chat shows as it is");
    CHECK(private_chat_policy_veiled(&secret, 2, &s), "but not through another account it was not unlocked on");
    CHECK(private_chat_policy_tui_searchable(&secret, 1, &s), "search may list the unlocked chat");
    CHECK(!private_chat_policy_needs_unlock(UNLOCK_FOR_OPEN, &secret, 1, &s), "opening it again does not ask");
    CHECK(private_chat_policy_needs_unlock(UNLOCK_FOR_MAKE_PUBLIC, &secret, 1, &s), "making it public asks even while it is unlocked");
    CHECK(private_chat_policy_needs_unlock(UNLOCK_FOR_EXPORT, &secret, 1, &s), "and so does exporting it");
    CHECK(private_chat_policy_needs_unlock(UNLOCK_FOR_PASSWORD_CHANGE, NULL, 1, &s), "changing the password always asks");

    private_chat_session_leave(&s, 1, B);
    CHECK(private_chat_policy_veiled(&secret, 1, &s), "leaving it veils it again");
}

int main(void) {
    session_rules();
    policy_rules();
    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: private chats unlock one at a time, lock again, and stay veiled and unsearchable while locked\n");
    return 0;
}
