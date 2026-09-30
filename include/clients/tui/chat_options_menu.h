#ifndef APP_CLIENTS_TUI_CHAT_OPTIONS_MENU_H
#define APP_CLIENTS_TUI_CHAT_OPTIONS_MENU_H

#include "clients/tui/chat_option.h"
#include "clients/tui/popup_result.h"
#include "clients/tui/ui_rect.h"
#include "core/chat.h"

/* Per-chat actions: mute, pin, archive, theme, tone, draft. */
typedef struct ChatOptionsMenu {
    int        open;
    char       jid[128];
    char       title[128];
    ChatOption items[CHAT_OPTION_COUNT];
    int        count;
    int        selected;
    int        row_item[64];
    UiRect     last_rect;
} ChatOptionsMenu;

void        chat_options_menu_open(ChatOptionsMenu *menu, const Chat *chat);
PopupResult chat_options_menu_key(ChatOptionsMenu *menu, int is_key_code, int ch);
PopupResult chat_options_menu_click(ChatOptionsMenu *menu, int y, int x);
ChatOption  chat_options_menu_choice(const ChatOptionsMenu *menu);
void        chat_options_menu_render(ChatOptionsMenu *menu, UiRect area);

#endif
