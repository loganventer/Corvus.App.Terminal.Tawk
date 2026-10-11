#ifndef APP_ENGINES_SUMMARY_POLICY_H
#define APP_ENGINES_SUMMARY_POLICY_H

#include "core/chat.h"
#include "core/chat_prefs.h"
#include "core/message.h"

/* Whether a chat's long messages are summarised at all: you switched TL;DR
 * on for it, and it is not locked or soft-locked. */
int summary_policy_allows(const Chat *chat, const ChatPrefs *prefs);
/* Whether this message is one to summarise: text that was not deleted and is
 * at least `min_chars` long, in a chat that allows it. */
int summary_policy_wants(const Chat *chat, const ChatPrefs *prefs, const Message *message, int min_chars);
/* The length a message must reach to be asked for: the larger of the length you chose (`from_chars`)
 * and the floor (`floor_chars`), below which no summary could be shorter than the message. */
int summary_policy_min_chars(int from_chars, int floor_chars);
/* Whether a summary is worth showing in place of its message: only when it
 * is shorter. One that is as long as the message, or longer, saves nothing,
 * so the message shows as it is. */
int summary_policy_shorter(const char *original, const char *summary);

#endif
