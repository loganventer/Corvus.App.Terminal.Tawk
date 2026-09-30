#ifndef APP_CLIENTS_TUI_CHAT_LIST_ENTRY_H
#define APP_CLIENTS_TUI_CHAT_LIST_ENTRY_H

#include "clients/tui/chat_list_entry_kind.h"

/* One selectable row of the chat list: a chat or a folder shortcut. */
typedef struct ChatListEntry {
    ChatListEntryKind kind;
    int               chat;      /* index into the chat array for CHAT entries */
    int               count;     /* chats in the folder or group */
    int               unread;    /* chats with unread messages in the folder or group */
} ChatListEntry;

#endif
