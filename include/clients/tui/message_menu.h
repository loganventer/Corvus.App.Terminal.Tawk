#ifndef APP_CLIENTS_TUI_MESSAGE_MENU_H
#define APP_CLIENTS_TUI_MESSAGE_MENU_H

#include "clients/tui/message_action.h"
#include "clients/tui/popup_result.h"
#include "clients/tui/ui_rect.h"

/* Right-click menu for one message, shown where the mouse is. */
typedef struct MessageMenu {
    int           open;
    int           message;              /* index into the loaded messages */
    MessageAction items[MESSAGE_ACTION_COUNT];
    int           count;
    int           selected;
    int           anchor_y;
    int           anchor_x;
    UiRect        last_rect;
} MessageMenu;

/* flags: which actions apply to this message. */
void          message_menu_open(MessageMenu *menu, int message, int y, int x, const int enabled[MESSAGE_ACTION_COUNT]);
PopupResult   message_menu_key(MessageMenu *menu, int is_key_code, int ch);
PopupResult   message_menu_click(MessageMenu *menu, int y, int x);
MessageAction message_menu_choice(const MessageMenu *menu);
void          message_menu_render(MessageMenu *menu, UiRect bounds);

#endif
