#ifndef APP_ENGINES_CHAT_MATCH_H
#define APP_ENGINES_CHAT_MATCH_H

#include "core/chat.h"

/* True when `filter` (typed text) matches the chat's name, case-insensitive,
 * or its JID. An empty filter matches every chat. */
int chat_match_filter(const Chat *chat, const char *filter);
/* Locked chats stay out of searches and pickers everywhere but the Locked
 * folder itself. */
int chat_match_searchable(const Chat *chat, int in_locked_folder);

#endif
