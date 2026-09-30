#ifndef APP_CLIENTS_TUI_ATTACH_MENU_H
#define APP_CLIENTS_TUI_ATTACH_MENU_H

#include "clients/tui/attach_choice.h"
#include "clients/tui/popup_result.h"
#include "clients/tui/ui_rect.h"

/* The + button's menu: take a photo or choose a file. */
typedef struct AttachMenu {
    int    open;
    int    has_camera;          /* without one, the photo item says so */
    int    selected;
    UiRect last_rect;
} AttachMenu;

void         attach_menu_open(AttachMenu *menu, int has_camera);
PopupResult  attach_menu_key(AttachMenu *menu, int is_key_code, int ch);
PopupResult  attach_menu_click(AttachMenu *menu, int y, int x);
AttachChoice attach_menu_choice(const AttachMenu *menu);
/* Drawn above the bottom right of `area`, next to the + button. */
void         attach_menu_render(AttachMenu *menu, UiRect area);

#endif
