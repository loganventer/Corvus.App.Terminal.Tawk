#ifndef APP_CLIENTS_TUI_CHAT_FOLDER_H
#define APP_CLIENTS_TUI_CHAT_FOLDER_H

/* Which chats the list shows. Archived and locked chats live in their own
 * folders and never appear among the regular chats. */
typedef enum ChatFolder {
    CHAT_FOLDER_CHATS = 0,
    CHAT_FOLDER_ARCHIVED,
    CHAT_FOLDER_LOCKED
} ChatFolder;

#endif
