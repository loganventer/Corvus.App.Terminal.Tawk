/* Private chats: which chat is unlocked, and when it locks again. */
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

int main(void) {
    session_rules();
    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: private chats unlock one at a time and lock again\n");
    return 0;
}
