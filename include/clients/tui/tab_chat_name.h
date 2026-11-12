#ifndef APP_CLIENTS_TUI_TAB_CHAT_NAME_H
#define APP_CLIENTS_TUI_TAB_CHAT_NAME_H

#include "core/chat.h"

/* What the terminal tab calls the open chat: its name, or a lock and no
 * name while it is veiled (a locked private chat, or tawk locked). */
const char *tab_chat_name(const Chat *chat, int veiled);

#endif
