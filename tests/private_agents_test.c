/* Agents see private chats like any other: the screen gate is for the
 * person at the keyboard. What keeps a chat from agents is its agent rule,
 * WhatsApp's Locked folder or the chats setting. TL;DR and transcription
 * still leave a private chat alone. */
#include "core/chat.h"
#include "core/chat_prefs.h"
#include "core/settings.h"
#include "engines/automation_policy.h"
#include "engines/chat_reference_resolver.h"
#include "engines/summary_policy.h"
#include "engines/transcription_policy.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(cond, what) do { if (!(cond)) { printf("FAIL: %s\n", what); failures++; } } while (0)

static Chat chat(const char *jid, const char *name, int is_private, int is_locked) {
    Chat c;
    chat_init(&c, jid);
    str_copy(c.name, sizeof(c.name), name);
    c.is_private = is_private;
    c.is_locked = is_locked;
    return c;
}

int main(void) {
    Settings s;
    settings_set_defaults(&s);
    Chat chats[3] = {
        chat("27820000001@s.whatsapp.net", "Diary", 1, 0),
        chat("27820000002@s.whatsapp.net", "Mom", 0, 0),
        chat("27820000003@s.whatsapp.net", "Vault", 0, 1),
    };
    CHECK(automation_policy_chat_allowed(&s, &chats[0]), "a private chat is open to agents");
    CHECK(!automation_policy_chat_allowed(&s, &chats[2]), "a chat in WhatsApp's Locked folder is not");

    int found = -1;
    CHECK(chat_reference_resolve(chats, 3, &s, "Diary", &found, NULL, 0, NULL) == CHAT_RESOLUTION_FOUND && found == 0,
          "an agent can name a private chat");
    CHECK(chat_reference_resolve(chats, 3, &s, "Vault", &found, NULL, 0, NULL) == CHAT_RESOLUTION_NOT_FOUND,
          "but not a Locked-folder one");

    str_copy(s.automation_chats, sizeof(s.automation_chats), "Mom");
    CHECK(!automation_policy_chat_allowed(&s, &chats[0]), "the chats setting still narrows what agents see");
    s.automation_chats[0] = '\0';

    ChatPrefs prefs;
    memset(&prefs, 0, sizeof(prefs));
    prefs.tldr = 1;
    CHECK(!summary_policy_allows(&chats[0], &prefs) && summary_policy_allows(&chats[1], &prefs), "TL;DR leaves a private chat alone");
    CHECK(!transcription_policy_allows(&chats[0], &prefs) && transcription_policy_allows(&chats[1], &prefs),
          "and so does transcription");

    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("ok: agents see private chats, while the agent rule, the Locked folder and the chats setting still keep chats from them\n");
    return 0;
}
